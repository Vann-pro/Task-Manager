// Tampilan frontend: merender data dari C++ dan meneruskan aksi pengguna.
(function () {
  'use strict';
  var $ = function (s) { return document.querySelector(s); };
  var ST = { todo: 'To Do', inprogress: 'In Progress', completed: 'Completed' };
  var PR = { high: 'High', medium: 'Medium', low: 'Low' };
  var filters = { status: 'all', priority: 'all', deadline: 'all', search: '', sort: 'created', dir: 'desc' };
  var editingId = null;
  var toastTimer = null;

  function el(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined) e.textContent = text;
    return e;
  }
  function toast(msg, isErr) {
    var t = $('#toast');
    t.textContent = msg;
    t.className = 'toast' + (isErr ? ' error' : '');
    t.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(function () { t.hidden = true; }, 4000);
  }
  function banner(msg) { var b = $('#banner'); b.textContent = msg; b.hidden = false; }
  function persist() {
    try { PTMStorage.save(PTMCore.state(Date.now())); }
    catch (e) { toast('Gagal menyimpan ke LocalStorage: ' + e.message, true); }
  }
  function dayBounds() {
    var s = new Date(); s.setHours(0, 0, 0, 0);
    var e = new Date(s); e.setHours(23, 59, 59, 999);
    return { dayStart: s.getTime(), dayEnd: e.getTime() };
  }
  function fmt(ms) {
    return new Date(ms).toLocaleString('id-ID', { day: '2-digit', month: 'short', year: 'numeric', hour: '2-digit', minute: '2-digit' });
  }
  function toLocalInput(ms) {
    var d = new Date(ms - new Date(ms).getTimezoneOffset() * 60000);
    return d.toISOString().slice(0, 16);
  }

  // ---------- Render ----------
  function renderStats(s) {
    var box = $('#stats');
    box.textContent = '';
    [['Total', s.total, ''], ['To Do', s.todo, ''], ['In Progress', s.inprogress, ''], ['Completed', s.completed, 'ok'],
     ['Terlambat', s.overdue, s.overdue ? 'bad' : ''], ['Prioritas High', s.high, s.high ? 'warn' : ''],
     ['Jatuh tempo hari ini', s.dueToday, ''], ['Selesai', s.completionRate + '%', '']].forEach(function (c) {
      var card = el('div', 'stat ' + c[2]);
      card.appendChild(el('div', 'stat-num', String(c[1])));
      card.appendChild(el('div', 'stat-label', c[0]));
      box.appendChild(card);
    });
  }

  function badge(text, cls) { return el('span', 'badge ' + cls, text); }

  function renderTask(t) {
    var card = el('article', 'task' + (t.overdue ? ' overdue' : '') + (t.status === 'completed' ? ' done' : ''));
    var head = el('div', 'task-head');
    head.appendChild(el('h3', 'task-title', t.title));
    var badges = el('div', 'badges');
    if (t.overdue) badges.appendChild(badge('Terlambat', 'overdue'));
    badges.appendChild(badge(ST[t.status], 's-' + t.status));
    badges.appendChild(badge(PR[t.priority] + (t.priorityMode === 'manual' ? ' · manual' : ' · auto'), 'p-' + t.priority));
    head.appendChild(badges);
    card.appendChild(head);
    if (t.description) card.appendChild(el('p', 'task-desc', t.description));

    var meta = el('div', 'task-meta');
    meta.appendChild(el('span', '', 'Deadline: ' + (t.deadline ? fmt(t.deadline) : 'tanpa deadline')));
    meta.appendChild(el('span', '', 'Dibuat: ' + fmt(t.createdAt)));
    meta.appendChild(el('span', '', 'Urgensi ' + t.urgency + ' · Kepentingan ' + t.importance + ' · Skor otomatis ' + t.autoScore.toFixed(1) + ' (' + PR[t.autoPriority] + ')'));
    card.appendChild(meta);

    var act = el('div', 'task-actions');
    var sel = el('select');
    sel.setAttribute('aria-label', 'Ubah status');
    Object.keys(ST).forEach(function (k) {
      var o = el('option', '', ST[k]); o.value = k; if (k === t.status) o.selected = true; sel.appendChild(o);
    });
    sel.addEventListener('change', function () {
      var copy = Object.assign({}, t, { status: sel.value });
      var r = PTMCore.update(copy, Date.now());
      if (!r.ok) toast(r.error, true); else persist();
      render();
    });
    var edit = el('button', 'btn ghost', 'Edit'); edit.type = 'button';
    edit.addEventListener('click', function () { openForm(t); });
    var del = el('button', 'btn danger', 'Hapus'); del.type = 'button';
    del.addEventListener('click', function () {
      if (!confirm('Hapus tugas "' + t.title + '"?')) return;
      var r = PTMCore.remove(t.id);
      if (!r.ok) toast(r.error, true); else { persist(); toast('Tugas dihapus.'); }
      render();
    });
    act.appendChild(sel); act.appendChild(edit); act.appendChild(del);
    card.appendChild(act);
    return card;
  }

  function render() {
    var now = Date.now();
    var res = PTMCore.query(Object.assign({}, filters, dayBounds()), now);
    renderStats(PTMCore.stats(now));
    var list = $('#list');
    list.textContent = '';
    if (!res.ok) { toast(res.error, true); return; }
    res.tasks.forEach(function (t) { list.appendChild(renderTask(t)); });
    $('#result-info').textContent = res.count + ' dari ' + res.total + ' tugas ditampilkan';
    var empty = $('#empty');
    empty.hidden = res.count > 0;
    empty.textContent = res.total === 0 ? 'Belum ada tugas. Tekan "+ Tugas" untuk memulai.' : 'Tidak ada tugas yang cocok dengan filter.';
  }

  // ---------- Form ----------
  function fillScale(sel) {
    for (var i = 1; i <= 5; i++) { var o = el('option', '', String(i)); o.value = String(i); sel.appendChild(o); }
  }
  function syncManual() { $('#t-manual').disabled = $('#t-mode').value !== 'manual'; }
  function openForm(t) {
    editingId = t ? t.id : null;
    $('#dlg-title').textContent = t ? 'Edit tugas' : 'Tugas baru';
    $('#t-title').value = t ? t.title : '';
    $('#t-desc').value = t ? t.description : '';
    $('#t-deadline').value = t && t.deadline ? toLocalInput(t.deadline) : '';
    $('#t-urgency').value = t ? t.urgency : 3;
    $('#t-importance').value = t ? t.importance : 3;
    $('#t-status').value = t ? t.status : 'todo';
    $('#t-mode').value = t ? t.priorityMode : 'auto';
    $('#t-manual').value = t ? t.manualPriority : 'medium';
    $('#form-error').hidden = true;
    syncManual();
    $('#dlg').showModal();
    $('#t-title').focus();
  }
  function submitForm(ev) {
    ev.preventDefault();
    var dl = $('#t-deadline').value;
    var task = {
      title: $('#t-title').value,
      description: $('#t-desc').value,
      deadline: dl ? new Date(dl).getTime() : null,
      urgency: +$('#t-urgency').value,
      importance: +$('#t-importance').value,
      status: $('#t-status').value,
      priorityMode: $('#t-mode').value,
      manualPriority: $('#t-manual').value
    };
    var r;
    if (editingId !== null) { task.id = editingId; r = PTMCore.update(task, Date.now()); }
    else r = PTMCore.add(task, Date.now());
    if (!r.ok) { var fe = $('#form-error'); fe.textContent = r.error; fe.hidden = false; return; }
    persist();
    $('#dlg').close();
    toast(editingId !== null ? 'Tugas diperbarui.' : 'Tugas ditambahkan.');
    render();
  }

  // ---------- Ekspor / Impor ----------
  function exportJson() {
    var blob = new Blob([PTMCore.exportData(Date.now())], { type: 'application/json' });
    var a = document.createElement('a');
    var d = new Date();
    var p = function (n) { return String(n).padStart(2, '0'); };
    a.href = URL.createObjectURL(blob);
    a.download = 'personal-task-manager-' + d.getFullYear() + '-' + p(d.getMonth() + 1) + '-' + p(d.getDate()) + '.json';
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(function () { URL.revokeObjectURL(a.href); }, 1000);
    toast('Data diekspor.');
  }
  function importFile(file) {
    if (!file) return;
    if (file.size > 10 * 1024 * 1024) { toast('Berkas terlalu besar (maks. 10 MB).', true); return; }
    var reader = new FileReader();
    reader.onerror = function () { toast('Gagal membaca berkas.', true); };
    reader.onload = function () {
      var text = String(reader.result);
      var v = PTMCore.importData(text, 0, Date.now());  // validasi dulu; data lama tidak disentuh
      if (!v.ok) { toast('Impor dibatalkan, berkas tidak valid: ' + v.error, true); return; }
      var mode = 0;
      if (confirm('Berkas valid (' + v.count + ' tugas).\n\nOK = GANTI seluruh data saat ini dengan isi berkas.\nBatal = lanjut ke pilihan lain.')) mode = 1;
      else if (confirm('Gabungkan ' + v.count + ' tugas dari berkas ke data saat ini?\n\nOK = gabungkan. Batal = batalkan impor.')) mode = 2;
      if (!mode) { toast('Impor dibatalkan.'); return; }
      var r = PTMCore.importData(text, mode, Date.now());
      if (!r.ok) { toast(r.error, true); return; }
      persist();
      render();
      toast(r.count + ' tugas ' + (mode === 1 ? 'dipulihkan.' : 'digabungkan.'));
    };
    reader.readAsText(file);
  }

  // ---------- Inisialisasi ----------
  function bind() {
    fillScale($('#t-urgency')); fillScale($('#t-importance'));
    $('#btn-add').addEventListener('click', function () { openForm(null); });
    $('#t-cancel').addEventListener('click', function () { $('#dlg').close(); });
    $('#form').addEventListener('submit', submitForm);
    $('#t-mode').addEventListener('change', syncManual);
    $('#btn-export').addEventListener('click', exportJson);
    $('#btn-import').addEventListener('click', function () { $('#file-input').click(); });
    $('#file-input').addEventListener('change', function (e) { importFile(e.target.files[0]); e.target.value = ''; });
    $('#btn-theme').addEventListener('click', function () { PTMStorage.setTheme(PTMStorage.getTheme() === 'dark' ? 'light' : 'dark'); });
    var searchTimer = null;
    $('#f-search').addEventListener('input', function (e) {
      clearTimeout(searchTimer);
      searchTimer = setTimeout(function () { filters.search = e.target.value; render(); }, 150);
    });
    [['#f-status', 'status'], ['#f-priority', 'priority'], ['#f-deadline', 'deadline'], ['#f-sort', 'sort']].forEach(function (p) {
      $(p[0]).addEventListener('change', function (e) {
        filters[p[1]] = e.target.value;
        if (p[1] === 'sort') filters.dir = e.target.value === 'deadline' ? 'asc' : 'desc';
        syncDir(); render();
      });
    });
    $('#f-dir').addEventListener('click', function () { filters.dir = filters.dir === 'asc' ? 'desc' : 'asc'; syncDir(); render(); });
  }
  function syncDir() {
    var asc = filters.dir === 'asc';
    var names = { created: ['Terlama ↑', 'Terbaru ↓'], deadline: ['Terdekat ↑', 'Terjauh ↓'], priority: ['Terendah ↑', 'Tertinggi ↓'] }[filters.sort];
    $('#f-dir').textContent = asc ? names[0] : names[1];
  }

  function init() {
    bind();
    syncDir();
    PTMCore.init().then(function () {
      var raw = PTMStorage.load();
      if (raw) {
        var r = PTMCore.load(raw, Date.now());
        if (!r.ok) {
          PTMStorage.backupCorrupt(raw);
          banner('Data tersimpan tidak dapat dibaca (' + r.error + '). Salinan aslinya disimpan di cadangan LocalStorage; aplikasi dimulai dari daftar kosong.');
        }
      }
      render();
      setInterval(render, 60000);  // perbarui penanda "terlambat"
    }).catch(function (e) {
      banner('Aplikasi tidak dapat dimulai: ' + e.message);
      document.querySelectorAll('button, input, select').forEach(function (x) { x.disabled = true; });
    });
  }
  init();
})();
