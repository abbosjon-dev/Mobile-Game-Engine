# Zamin Game Engine — Reja (Android)

**Maqsad:** Android qurilmaning o'zida 2D/3D o'yin yaratish, ishga tushirish (test), va tayyor APK qilib build qilish imkonini beruvchi game engine + editor. Unity/Unreal'dagi asosiy imkoniyatlar (scene editor, kod, blueprint/vizual skript, assetlar, terrain, fizika, audio) — sensorli ekranga moslashtirilgan holda.

## 0. Qabul qilingan qarorlar

| Savol | Qaror |
|---|---|
| Nomi | **Zamin Game Engine** |
| Skript tili | **Luau** (oddiy, qulay, tez) + Blueprint vizual skript |
| 2D / 3D | Ikkalasi **boshidan birga** (bitta sahna tizimi, 2D = ortografik kamera + sprite) |
| Qurilmalar | Telefon va planshet (moslashuvchan layout) |
| Minimal Android | **Android 10 (API 29)** — Vulkan 1.1 kafolatlangan, GLES zaxira kerak emas |
| Grafika | Faqat **Vulkan** |

---

## 1. Asosiy arxitektura qarorlari

### 1.1 Uch qatlam

```
┌──────────────────────────────────────────────────────────┐
│  EDITOR (Kotlin + Jetpack Compose)                       │
│  Panellar, inspector, asset browser, kod muharriri,      │
│  blueprint graph muharriri, terrain asboblari, build UI  │
├──────────────────────────────────────────────────────────┤
│  JNI ko'prik (editor ↔ engine buyruqlari, viewport)      │
├──────────────────────────────────────────────────────────┤
│  ENGINE CORE (C++20, Android NDK)                        │
│  ECS · Renderer · Fizika · Audio · Skript VM · Assetlar  │
└──────────────────────────────────────────────────────────┘
          │ bir xil core
          ▼
┌──────────────────────────────────────────────────────────┐
│  RUNTIME PLAYER (shablon APK) — editor'siz, faqat o'yin  │
└──────────────────────────────────────────────────────────┘
```

- **Engine core C++ da** — tezlik, Vulkan/GLES ga to'g'ridan-to'g'ri kirish, kelajakda iOS/desktop'ga ko'chirish oson.
- **Editor UI Kotlin/Compose da** — sensorli boshqaruv, panellar, klaviaturasiz ishlash uchun Android'ning tayyor UI tizimi eng qulay. Viewport esa `SurfaceView` ichida native engine tomonidan chiziladi.

### 1.2 Eng muhim qaror: qurilmada "build" qanday ishlaydi

Telefonda C++ kompilyatsiya qilish va Gradle ishga tushirish amalda imkonsiz/juda og'ir. Shuning uchun:

1. **O'yin logikasi interpretatsiya qilinadi** — matnli skript (Lua/Luau) va blueprint graflar engine ichidagi VM'da ishlaydi. Native kompilyatsiya kerak emas.
2. **Oldindan tayyorlangan "Player" shablon APK** editor ichiga joylanadi (engine core + skript VM, o'yin ma'lumotisiz).
3. **Build jarayoni (qurilmada):**
   - Assetlarni optimallashtirish (teksturalar → ASTC/ETC2, mesh → binar format, sahnalar → binar paket)
   - Skriptlarni bytecode'ga kompilyatsiya (VM o'zi qila oladi)
   - `game.pak` faylini shablon APK ichiga qo'shish (APK = ZIP)
   - `AndroidManifest.xml` (binar XML) da paket nomi, versiya, o'yin nomini o'zgartirish; ikonka almashtirish
   - `apksig` kutubxonasi bilan imzolash (v2/v3 signature), foydalanuvchi keystore'i bilan
   - `PackageInstaller` API orqali o'rnatish yoki faylni ulashish
4. **Test (Play mode):** editor ichida o'sha zahoti — sahna nusxasi olinadi, o'yin ishga tushadi, to'xtatilganda holat qaytariladi. APK build qilish shart emas.

> Bu yondashuv Google Play talablari uchun ham mos (alohida AAB eksport keyinroq qo'shiladi).

### 1.3 Texnologiya steki (tavsiya)

| Qism | Tanlov | Sabab |
|---|---|---|
| Til (core) | C++20, NDK r27+, CMake | Standart, tez |
| Grafika | **Vulkan 1.1** (RHI abstraksiya qatlami orqali, kelajakda Metal/D3D) | Android 10+ da kafolatlangan |
| Shader | GLSL → SPIR-V (oldindan), runtime'da `glslang` (foydalanuvchi shaderlari uchun) | Qurilmada shader kompilyatsiya |
| ECS | EnTT | Yetuk, header-only |
| 2D fizika | Box2D v3 | Sanoat standarti |
| 3D fizika | Jolt Physics | Zamonaviy, mobilga mos, tez |
| Audio | miniaudio (Oboe/AAudio backend) | Bitta header, past kechikish |
| Skript | **Luau** (yoki Lua 5.4) | Xavfsiz sandbox, tez VM, tiplar |
| Vizual skript | O'z blueprint tizimimiz → Luau bytecode yoki graf-interpretator | Unreal Blueprint analogi |
| Model import | cgltf (glTF 2.0), ufbx (FBX), stb_image | Engil, bog'liqliksiz |
| Tekstura siqish | basis_universal / astc-encoder | ASTC/ETC2 qurilmada |
| Serializatsiya | JSON (editor, git-friendly) + binar (runtime) | |
| Editor UI | Kotlin, Jetpack Compose | Sensorli UI |
| Kod muharriri | Sora Editor (Android) + Luau LSP-lite | Syntax highlight, autocomplete |
| APK imzolash | `apksig` (AOSP) | Qurilmada imzolash |
| Engine debug UI | Dear ImGui (faqat ichki overlay/profiler) | |

---

## 2. Modullar ro'yxati

**Engine core**
- `core` — xotira, logging, fayl tizimi (SAF/Android assets), vaqt, job system (ko'p yadroli)
- `rhi` — Vulkan/GLES abstraksiya
- `renderer` — 2D sprite batcher, 3D forward+ PBR, soyalar (CSM), post-process (bloom, tonemap, FXAA), sky
- `scene` — ECS, transform ierarxiyasi, prefablar, sahna yuklash/saqlash
- `physics2d`, `physics3d`
- `audio` — 2D/3D tovush, mikser
- `animation` — sprite animatsiya, skeletal (glTF skin), blend tree, tween
- `terrain` — heightmap, chunk LOD, splat (4–8 qatlam), daraxt/o't instancing
- `particles` — CPU (keyin GPU) particle tizimi
- `ui` — o'yin ichidagi UI (canvas, tugma, matn, anchor)
- `input` — touch, multitouch gesture, virtual joystick, gamepad, akselerometr
- `script` — Luau VM, API bindinglari, hot-reload
- `blueprint` — node graf runtime
- `assets` — import pipeline, GUID asosidagi asset bazasi, kesh

**Editor**
- Loyiha menejeri (yaratish, ochish, shablonlar: 2D platformer, 3D FPS, bo'sh)
- Viewport (gizmo: ko'chirish/aylantirish/masshtab, sensorli kamera boshqaruvi)
- Sahna ierarxiyasi, Inspector (komponent xususiyatlari)
- Asset browser (import: fayl menejeri, galereya, ZIP)
- Kod muharriri (Luau)
- Blueprint muharriri (node canvas, pinch-zoom, ulanishlar)
- Terrain asboblari (ko'tarish/tushirish/silliqlash/bo'yash cho'tkalari)
- Sprite/Tilemap muharriri (2D)
- Animatsiya timeline
- Material muharriri (keyinroq node-based)
- Play / Pause / Step, konsol, profiler
- Build sozlamalari va APK eksport

---

## 3. Bosqichlar (milestone'lar)

### Bosqich 0 — Poydevor (2–3 hafta)
- [x] Repo tuzilmasi, CMake + Gradle, NDK sozlash, CI (GitHub Actions: APK build)
- [x] Android app skeleti: Compose editor + `SurfaceView` + JNI ko'prik
- [x] Moslashuvchan layout: planshet (yonma-yon panellar) / telefon (pastki navigatsiya)
- [x] `core`: logging, render oqimi, vaqt/FPS
- [ ] `core`: fayl tizimi, job system
- [x] `rhi`: Vulkan ishga tushirish, swapchain (pre-rotation), depth buffer, 3D piramida + touch bilan aylantirish
- [ ] Haqiqiy qurilmalarda sinov
- **Natija:** telefonda ilova ochiladi, native viewport ishlaydi

### Bosqich 1 — Sahna + 2D/3D asoslari (8–10 hafta)
- [ ] ECS + Transform + sahna saqlash/yuklash (JSON)
- [ ] Sprite renderer (batching, atlas), mesh renderer (glTF), kamera 2D (ortografik) va 3D (perspektiv)
- [ ] Touch input, virtual joystick
- [ ] Box2D (2D) va Jolt (3D) fizika integratsiyasi
- [ ] Luau skript: `onStart`, `onUpdate`, `onCollision` + asosiy API
- [ ] Editor: ierarxiya, inspector, viewport gizmo, asset import (PNG, OGG)
- [ ] Play mode (editor ichida test)
- [ ] Audio
- **Natija:** telefonda oddiy 2D yoki 3D o'yin yaratib, editor ichida o'ynash mumkin

### Bosqich 2 — Qurilmada APK build (3–4 hafta)
- [ ] Runtime Player shablon APK (CI'da build qilinib editor ichiga joylanadi)
- [ ] Asset paketlash (`game.pak`)
- [ ] Binar manifest tahrirlash (paket nomi, ilova nomi, versiya), ikonka
- [ ] Keystore yaratish/saqlash, `apksig` bilan imzolash
- [ ] O'rnatish (`PackageInstaller`) va ulashish
- **Natija:** to'liq sikl — yarat → test qil → APK → o'rnat. **Bu birinchi muhim demo.**

### Bosqich 3 — 3D kengaytirilgan (8–10 hafta)
- [ ] PBR material, yorug'lik (directional/point/spot), soyalar
- [ ] Character controller, raycast, trigger'lar
- [ ] Skybox, post-process
- [ ] 3D gizmo va sensorli orbit/fly kamera editorda
- [ ] Skeletal animatsiya
- **Natija:** oddiy 3D o'yin (masalan, 3rd-person yugurish) yaratish mumkin

### Bosqich 4 — Blueprint (vizual skript) (6–8 hafta)
- [ ] Graf ma'lumot modeli (node, pin, tiplar, ulanishlar)
- [ ] Event node'lar (Start, Update, Touch, Collision), flow control (Branch, Loop, Sequence, Delay)
- [ ] Engine API'dan avtomatik node generatsiya (bitta reflection tizimi: Luau va Blueprint uchun umumiy)
- [ ] Sensorli graf muharriri (pinch-zoom, sudrab ulash, qidiruv bilan node qo'shish)
- [ ] Blueprint debug: ishlayotgan node'larni yoritish, breakpoint
- **Natija:** kod yozmasdan o'yin logikasi

### Bosqich 5 — Terrain va ochiq dunyo (5–6 hafta)
- [ ] Heightmap terrain, chunk + LOD
- [ ] Sculpt cho'tkalari (raise/lower/smooth/flatten), touch bosimi/o'lcham
- [ ] Tekstura qatlamlarini bo'yash (splatmap)
- [ ] Daraxt/o't/tosh instancing (foliage painting)
- [ ] Terrain collider (Jolt HeightField)

### Bosqich 6 — Polishing va qo'shimcha (davomiy)
- [ ] Particle tizimi, o'yin UI tizimi, tilemap muharriri
- [ ] Animatsiya timeline, animator state machine
- [ ] Node-based material muharriri
- [ ] Prefab variantlari, undo/redo hamma joyda
- [ ] Profiler, xotira/FPS statistikasi
- [ ] Loyihani eksport/import (ZIP), bulutga zaxira
- [ ] AAB eksport (Google Play), reklama/IAP plaginlari
- [ ] Keyinchalik: iOS, desktop editor (bir xil core)

---

## 4. Repo tuzilmasi (taklif)

```
Mobile-Game-Engine/
├── engine/                 # C++ core
│   ├── core/  rhi/  renderer/  scene/  physics/  audio/
│   ├── script/  blueprint/  terrain/  assets/  ui/  input/
│   └── CMakeLists.txt
├── thirdparty/             # EnTT, Box2D, Jolt, Luau, miniaudio, cgltf ...
├── android/
│   ├── editor/             # Kotlin + Compose editor ilovasi
│   ├── player/             # Runtime player (shablon APK)
│   └── engine-jni/         # JNI ko'prik
├── shaders/                # GLSL manbalari
├── templates/              # O'yin shablonlari (2D platformer, 3D ...)
├── docs/                   # Arxitektura hujjatlari
└── tools/                  # Desktop yordamchi skriptlar (CI uchun)
```

---

## 5. Asosiy xavflar va yechimlar

| Xavf | Yechim |
|---|---|
| Qurilmalar xilma-xilligi (GPU drayver xatolari) | Vulkan + GLES zaxira, minimal API 26 (Android 8), turli qurilmalarda test |
| Kichik ekranda murakkab editor | Planshetga moslashuvchan layout, telefonda tab/drawer panellar, gesture'lar |
| Ishlash tezligi (editor + o'yin bir vaqtda) | Play mode'da editor panellari to'xtatiladi, job system |
| Katta loyihalar xotirasi | Asset streaming, GUID asosida lazy-load |
| Android fayl ruxsatlari (scoped storage) | Loyihalar ilova ichki xotirasida, import/eksport SAF orqali |
| Hajm (scope) juda katta | Qat'iy bosqichma-bosqich: avval 2D + build sikli, keyin 3D |

---

## 6. Birinchi qadamlar (hozir)

1. Ushbu rejani tasdiqlash / o'zgartirish (stek, minimal Android versiya, skript tili)
2. **Bosqich 0** ni boshlash: Gradle + CMake skelet, Compose editor oynasi, `SurfaceView` da Vulkan bilan rangli ekran
3. CI sozlash (har push'da APK build)
