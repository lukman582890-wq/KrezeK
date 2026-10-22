# Octave Havoc (VST3)

Efek octave-up gaya "chaos" untuk gitar. Desain orisinal, dibuat berdasarkan deskripsi fungsional
sebuah manual publik (bukan hasil membongkar kode produk lain).

## Kontrol
| Kontrol | Fungsi |
|---|---|
| OCTAVE +1 / +2 | Footswitch (momentary; klik kanan -> Latch). Bisa diotomasi di DAW. |
| RISE | Waktu glide menuju pitch penuh: 5 ms - 2 s, atau tempo sync (1/32 ... 2 bar) lewat tombol TEMPO SYNC |
| PANIC | Detune (sampai 100 cent; voice +1 oktaf ke atas, voice +2 oktaf ke bawah) |
| CHAOS | Gerak pitch acak (sampai +/-500 cent) |
| SPEED | LFO amplitudo + all-pass feedback |
| INPUT / OUTPUT | Gain -24..+24 dB |
| GATE | Noise gate pada input (OFF di -80 dB) |
| BLEND | Dry/wet |
| STEREO | Mono-sum atau proses dua kanal |

## Build
### Cara 1: GitHub Actions (tanpa install apa pun)
1. Buat repo GitHub, push seluruh folder ini.
2. Tab **Actions** -> workflow "Build VST3 (Windows)" -> tunggu selesai.
3. Unduh artifact `OctaveHavoc-VST3-Windows` -> berisi folder `Octave Havoc.vst3`.

### Cara 2: Lokal (Windows)
Butuh Visual Studio 2022 (workload "Desktop development with C++"), CMake, dan Git.
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target OctaveHavoc_VST3
```
Hasil: `build/OctaveHavoc_artefacts/Release/VST3/Octave Havoc.vst3`

## Instal
Salin folder `Octave Havoc.vst3` ke `C:\Program Files\Common Files\VST3\`, lalu scan ulang plugin di DAW.
Plugin muncul sebagai **HavocLab > Octave Havoc**.

## Lisensi JUCE
JUCE dilisensikan AGPLv3 / komersial. Untuk distribusi publik/komersial periksa lisensi JUCE.
