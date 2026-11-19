// Pengelolaan LocalStorage. Tidak berisi logika tugas.
(function () {
  'use strict';
  var KEY = 'ptm.state.v1', CORRUPT = 'ptm.corrupt.backup', THEME = 'ptm.theme';

  window.PTMStorage = {
    load: function () {
      try { return localStorage.getItem(KEY) || ''; } catch (e) { return ''; }
    },
    save: function (json) {
      localStorage.setItem(KEY, json);  // melempar error jika penuh/diblokir; ditangani pemanggil
    },
    backupCorrupt: function (raw) {
      try { localStorage.setItem(CORRUPT, raw); } catch (e) {}
    },
    getTheme: function () { return document.documentElement.getAttribute('data-theme') || 'light'; },
    setTheme: function (t) {
      document.documentElement.setAttribute('data-theme', t);
      try { localStorage.setItem(THEME, t); } catch (e) {}
    }
  };
})();
