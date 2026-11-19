# Personal Task Manager

Aplikasi pengelola tugas berbasis web. Logika utama ditulis dalam **C++**, dikompilasi ke **WebAssembly** dengan Emscripten. Data disimpan di **LocalStorage** browser. Tanpa login, tanpa server, tanpa biaya.

## STATUS JUJUR (baca dulu)

Lingkungan tempat proyek ini dibuat **tidak punya Emscripten dan tidak punya akses internet**, sehingga:

| Bagian | Status |
|---|---|
| Logika C++ (prioritas, CRUD, filter, urut, statistik, JSON, validasi impor) | **Sudah diuji** secara native dengan g++ + sanitizer: 74 pemeriksaan lulus (`scripts/test.sh`) |
| Kompilasi ke WebAssembly (`emcc`) | **Belum dijalankan.** Perintah build ditulis tetapi belum pernah dieksekusi |
| File `.wasm` / `web/wasm/ptm.js` | **Belum ada di ZIP** (tidak mungkin dibuat tanpa Emscripten). Dibuat otomatis oleh langkah di bawah |
| JavaScript + HTML + CSS | Sintaks sudah diperiksa (`node --check`). **Belum dijalankan di browser** |
| Responsivitas, tema, LocalStorage lintas sesi | **Belum diuji di browser** |

Jadi: aplikasi baru berjalan setelah langkah build di bawah. Jika ada galat saat build atau di browser, kirimkan pesan galatnya agar bisa diperbaiki.

## Cara 1 (paling mudah): build otomatis di GitHub, tanpa memasang apa pun

1. Buat repositori GitHub baru (publik) dan unggah seluruh isi folder ini (termasuk folder `.github`).
2. Di repositori: **Settings > Pages > Build and deployment > Source: GitHub Actions**.
3. Buka tab **Actions**. Workflow "Build dan Publikasi ke GitHub Pages" akan menjalankan uji C++, mengompilasi WebAssembly, lalu mempublikasikan folder `web/`.
4. Setelah hijau, aplikasi tersedia di `https://<username>.github.io/<nama-repo>/`.

Jika workflow gagal, buka log langkah yang merah dan kirimkan isinya.

## Cara 2: build di komputer sendiri

1. Pasang Emscripten (gratis): https://emscripten.org/docs/getting_started/downloads.html
2. Aktifkan di terminal (`emsdk_env.bat` di Windows, `source emsdk_env.sh` di Linux/macOS).
3. Build:
   - Windows: `scripts\build.bat`
   - Linux/macOS: `bash scripts/build.sh`
4. Hasil: `web/wasm/ptm.js` (WebAssembly tertanam di dalamnya, opsi `SINGLE_FILE`).

## Menjalankan secara lokal

Karena WebAssembly sudah tertanam di `ptm.js`, `web/index.html` seharusnya bisa dibuka langsung (`file://`). Namun sebagian browser membatasi LocalStorage pada `file://`, jadi lebih aman memakai server lokal gratis:

- Windows: `scripts\serve.bat` (butuh Python 3) lalu buka http://localhost:8080
- Linux/macOS: `bash scripts/serve.sh`

## Struktur

```
src/task.h/.cpp          Struktur data tugas
src/priority.h/.cpp      Perhitungan prioritas
src/json.h/.cpp          Parser/penulis JSON minimal
src/task_manager.h/.cpp  CRUD, validasi, filter, urut, statistik, ekspor/impor
src/wasm_api.cpp         Fungsi yang diekspor ke WebAssembly
tests/test_core.cpp      Uji native logika C++
web/index.html, css/     Tampilan
web/js/wasm-bridge.js    Jembatan JS <-> WASM
web/js/storage.js        LocalStorage dan tema
web/js/ui.js             Render dan interaksi
scripts/                 build, test, serve (Windows dan Linux)
.github/workflows/       Build otomatis + GitHub Pages
```

## Sistem prioritas

`P = 0,4U + 0,3K + 0,3D` (dibulatkan 1 desimal). High 4,0-5,0, Medium 2,5-3,9, Low 1,0-2,4.

Skor kedekatan deadline D: tanpa deadline = 1; terlambat atau sisa <= 24 jam = 5; <= 3 hari = 4; <= 7 hari = 3; <= 14 hari = 2; lebih jauh = 1.

Aturan khusus:
- **Tanpa deadline**: D = 1 (tidak mendorong prioritas naik).
- **Terlambat** (belum selesai): D = 5 dan ditandai "Terlambat".
- **Selesai**: prioritas otomatis selalu Low dan tidak dianggap terlambat.
- **Mode manual**: prioritas yang Anda pilih selalu dipakai. Skor otomatis tetap ditampilkan sebagai informasi, tidak pernah menimpa pilihan manual.

## Penyimpanan, ekspor, impor

- Setiap perubahan otomatis disimpan ke LocalStorage (kunci `ptm.state.v1`).
- Ekspor menghasilkan file `personal-task-manager-YYYY-MM-DD.json`.
- Impor memvalidasi seluruh berkas lebih dulu (judul, skala 1-5, status, id ganda, tipe data, batas panjang). Jika ada satu saja yang tidak valid, **seluruh impor dibatalkan** dan data lama tidak berubah. Anda lalu memilih Ganti semua atau Gabungkan.
- Jika data di LocalStorage rusak saat dimuat, salinan aslinya disimpan di kunci `ptm.corrupt.backup`.
- Data tidak pernah dikirim ke server. Catatan: LocalStorage terikat pada browser dan alamat situs; menghapus data situs akan menghapusnya, jadi lakukan ekspor berkala.

## Panduan pengujian

**A. Uji C++ otomatis:** `bash scripts/test.sh` (atau `scripts\test.bat`). Harus menampilkan `74 lulus, 0 gagal`.

**B. Uji manual di browser** (setelah build). Centang satu per satu:
1. Halaman terbuka tanpa banner merah; kartu statistik tampil semua 0.
2. **Tambah**: "+ Tugas", isi judul, simpan. Tugas muncul, statistik Total = 1.
3. **Validasi**: simpan dengan judul kosong. Muncul pesan galat, tugas tidak bertambah.
4. **Edit**: ubah judul/deadline/urgensi. Perubahan tampil; tanggal dibuat tidak berubah.
5. **Status**: ubah lewat menu pada kartu ke In Progress lalu Completed. Statistik ikut berubah.
6. **Prioritas otomatis**: urgensi 5, kepentingan 5, deadline besok. Harus tampil High (skor 5,0). Tanpa deadline, 5/5 menghasilkan 3,8 (Medium).
7. **Prioritas manual**: pilih mode Manual, level Low pada tugas urgensi 5/5. Badge tetap "Low · manual" walau skor otomatis tinggi.
8. **Terlambat**: buat tugas dengan deadline beberapa menit lalu (atau edit). Muncul badge dan garis merah "Terlambat"; angka Terlambat di statistik naik. Tandai Completed, penanda hilang.
9. **Filter**: coba status, prioritas, dan deadline (Terlambat, Hari ini, 7 hari, Tanpa deadline).
10. **Cari**: ketik sebagian judul (huruf besar/kecil tidak berpengaruh).
11. **Urut**: tanggal dibuat, deadline (tanpa deadline di akhir), prioritas; tombol arah membalik urutan.
12. **Hapus**: konfirmasi muncul; tugas hilang.
13. **Persisten**: tutup tab/browser, buka lagi. Semua tugas masih ada, tema juga diingat.
14. **Ekspor**: unduh JSON dan buka isinya.
15. **Impor valid**: hapus semua tugas, impor file tadi, pilih Ganti. Data kembali. Coba juga Gabungkan.
16. **Impor rusak**: ubah file jadi `{"tasks":[{"title":123}]}` atau teks acak. Muncul pesan galat dan data lama utuh.
17. **Tema**: tombol Tema berganti terang/gelap.
18. **Responsif**: kecilkan jendela hingga ~360 px (atau mode perangkat di DevTools). Tidak ada scroll horizontal, tombol tetap mudah ditekan.
19. **Konsol**: buka DevTools > Console; tidak boleh ada galat merah.

## Daftar fitur

Kode sudah ditulis untuk semua fitur wajib. Kolom status menunjukkan apa yang benar-benar sudah diverifikasi.

| Fitur | Ditulis | Diverifikasi |
|---|---|---|
| Dashboard statistik | Ya | Logika C++ diuji; tampilan belum dilihat di browser |
| CRUD tugas | Ya | Logika C++ diuji; form UI belum |
| Judul, deskripsi, dibuat, deadline, urgensi, kepentingan, status, prioritas | Ya | C++ diuji |
| Status To Do / In Progress / Completed | Ya | C++ diuji |
| Prioritas otomatis | Ya | C++ diuji (batas kelas, tanpa deadline, terlambat, selesai) |
| Prioritas manual tidak tertimpa | Ya | C++ diuji |
| Filter status, prioritas, deadline | Ya | C++ diuji |
| Pencarian judul | Ya | C++ diuji |
| Pengurutan | Ya | C++ diuji |
| Penanda terlambat | Ya | C++ diuji; tampilan belum |
| Ekspor JSON | Ya | Serialisasi diuji; unduhan di browser belum |
| Impor/pemulihan JSON + validasi atomik | Ya | C++ diuji (9 jenis berkas rusak); pemilih berkas belum |
| Mode terang/gelap | Ya | Belum diuji di browser |
| Responsif | Ya | Belum diuji di browser |
| Simpan otomatis ke LocalStorage | Ya | Belum diuji di browser |
| Kompilasi WebAssembly | Skrip siap | **Belum pernah dijalankan** |
| Build otomatis GitHub Pages | Workflow siap | **Belum pernah dijalankan** |

Belum ada: kategori/label, tugas berulang, notifikasi pengingat, undo, uji otomatis untuk sisi JavaScript.
