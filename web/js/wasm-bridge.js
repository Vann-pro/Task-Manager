// Jembatan JavaScript <-> WebAssembly. Seluruh logika ada di C++; berkas ini hanya meneruskan data.
(function () {
  'use strict';
  var fn = null;

  function call(name, args) {
    var out = fn[name].apply(null, args);
    return JSON.parse(out);
  }

  window.PTMCore = {
    init: function () {
      if (typeof createPTMModule !== 'function') {
        return Promise.reject(new Error(
          'Berkas WebAssembly (web/wasm/ptm.js) tidak ditemukan. Jalankan scripts/build.bat atau scripts/build.sh terlebih dahulu (lihat README).'));
      }
      return createPTMModule().then(function (mod) {
        var w = function (n, r, a) { return mod.cwrap('ptm_' + n, r, a); };
        fn = {
          load: w('load', 'string', ['string', 'number']),
          add: w('add', 'string', ['string', 'number']),
          update: w('update', 'string', ['string', 'number']),
          remove: w('delete', 'string', ['number']),
          query: w('query', 'string', ['string', 'number']),
          stats: w('stats', 'string', ['number']),
          state: w('state', 'string', ['number']),
          exportData: w('export', 'string', ['number']),
          importData: w('import', 'string', ['string', 'number', 'number'])
        };
      });
    },
    load: function (text, now) { return call('load', [text, now]); },
    add: function (task, now) { return call('add', [JSON.stringify(task), now]); },
    update: function (task, now) { return call('update', [JSON.stringify(task), now]); },
    remove: function (id) { return call('remove', [id]); },
    query: function (filter, now) { return call('query', [JSON.stringify(filter), now]); },
    stats: function (now) { return call('stats', [now]); },
    state: function (now) { return fn.state(now); },            // string JSON untuk LocalStorage
    exportData: function (now) { return fn.exportData(now); },  // string JSON untuk berkas
    importData: function (text, mode, now) { return call('importData', [text, mode, now]); }
  };
})();
