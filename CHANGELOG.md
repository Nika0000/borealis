# Changelog

## [1.4.0](https://github.com/Nika0000/borealis/compare/v1.3.0...v1.4.0) (2026-07-23)


### Features

* add frame deadline tracking for Android vsync timing ([e583415](https://github.com/Nika0000/borealis/commit/e583415ccf56a5611ce7840eaddae70b76968f95))
* add hover gesture to slider ([6b1e8e4](https://github.com/Nika0000/borealis/commit/6b1e8e48f16c4e76ea230d3d1dae3c1b8d3108e9))
* add removeGestureRecognizer method to View ([dd86f9c](https://github.com/Nika0000/borealis/commit/dd86f9cae6060edb15113c0a400b7a022af8ffb6))
* Improve slider UX with animated thumb scaling ([b55c4e8](https://github.com/Nika0000/borealis/commit/b55c4e84ad43d98d59a96d4e35eb8e9eeef9b0ae))


### Bug Fixes

* restore window geometry after exiting fullscreen on macOS ([cde9a72](https://github.com/Nika0000/borealis/commit/cde9a7272bd72dc7b95e9df30faa38285bf605af))

## [1.3.0](https://github.com/Nika0000/borealis/compare/v1.2.0...v1.3.0) (2026-07-12)


### Features

* add focusTarget xml attribute to view ([3d97d6f](https://github.com/Nika0000/borealis/commit/3d97d6fbf9b159af70b6a2255335e24a42bbf7a0))
* add linear gradient background api to View ([4c49271](https://github.com/Nika0000/borealis/commit/4c4927143f90a0561a66919e562ba18137d9eee3))
* add START and END scrolling behaviors ([0f3b573](https://github.com/Nika0000/borealis/commit/0f3b5739f10474fd956d5d1f4a6866bb4fb58f3f))
* add Vulkan backend support for Windows and Linux platforms ([51f585b](https://github.com/Nika0000/borealis/commit/51f585be935693c5a99e99e53fed8b80c0f8af77))
* add Vulkan validation layer debugging with debug messenger ([3a79bd9](https://github.com/Nika0000/borealis/commit/3a79bd938cef3b12e76ec3033ff871fd83e6a409))
* detect D3D11 flip-model occlusion via Z-order polling ([bd4e255](https://github.com/Nika0000/borealis/commit/bd4e2552099c76dcbf4d9cff6309e5b1a45ccb34))
* update audio files for system sounds ([0180cb6](https://github.com/Nika0000/borealis/commit/0180cb63865f72f234e9f9d66aef777c26403a55))


### Bug Fixes

* disable RC controller as joystick and fix text input on TV ([14b428a](https://github.com/Nika0000/borealis/commit/14b428ad85784162079682de0efff603f2a597d2))
* enable classic errata for Yoga 3 layout compatibility ([ea02657](https://github.com/Nika0000/borealis/commit/ea026576e5d36b7905dc388c0a4435f00e0a43a7))
* fix missing shader includes for nanovg ([9ccfc98](https://github.com/Nika0000/borealis/commit/9ccfc9876006a3d0170f7cb52b46d8eaee4c49c6))
* style metrics for custom notification width ([d234925](https://github.com/Nika0000/borealis/commit/d2349257b70f80818aed047b67f2629db5a6defb))
* throttle background event loop to deactivated fps to prevent cpu spinning ([4a968c8](https://github.com/Nika0000/borealis/commit/4a968c8d37f4f34e719fcfbf9ca59a42c77ae551))
* touch gesture plays focus sound along with click sound ([fad21df](https://github.com/Nika0000/borealis/commit/fad21dfefc00da19a9d5a3285b90af9b3b86b1da))

## [1.2.0](https://github.com/Nika0000/borealis/compare/v1.1.0...v1.2.0) (2026-06-13)


### Features

* add D3D11 frame latency waitable object for non-blocking vsync ([a6fe8fb](https://github.com/Nika0000/borealis/commit/a6fe8fb9b8d4d64de3653b0bf0400928c96d44ef))
* add per-player controller icons and refactor status bar widgets ([2babfdc](https://github.com/Nika0000/borealis/commit/2babfdc60a21ab467f47526c51241bcb3758b595))
* add wasm build and deployment support ([a3e9208](https://github.com/Nika0000/borealis/commit/a3e9208793e1ec6726f057607c915d234379f837))
* enable scroll animation in nested scroll views ([83fa039](https://github.com/Nika0000/borealis/commit/83fa0390bd2b20ec23e3513174d6873d6d1331f6))


### Bug Fixes

* android fps limiter using choreographer frame skipping instead of spin-wait ([494a0e5](https://github.com/Nika0000/borealis/commit/494a0e562b6b384b40b8e62db29f41f9e550aeef))
* correct artifact path in wasm deploy workflow ([1c9b263](https://github.com/Nika0000/borealis/commit/1c9b26317aa40de2adc57c38fd0c6135bf55dce2))
* correct versionCode ordering in build.gradle ([1e5e03b](https://github.com/Nika0000/borealis/commit/1e5e03b9e3536ebde7043498124d1eebe5eb3922))
* keep rendering while finite animations/timers are running ([f8f5b7a](https://github.com/Nika0000/borealis/commit/f8f5b7af06b0a3fa97ab2442f56a423465afb97a))
* link missing android native library ([dcf7953](https://github.com/Nika0000/borealis/commit/dcf7953de56d1924049b0ed0408681bfe9a20d19))

## [1.1.0](https://github.com/Nika0000/borealis/compare/v1.0.0...v1.1.0) (2026-06-11)


### Features

* add d3d11 feature level 12 ([174c21b](https://github.com/Nika0000/borealis/commit/174c21b0200057995eabbba9caa1bc1809c5c52e))
* add dx11.1 ([0fe6e6a](https://github.com/Nika0000/borealis/commit/0fe6e6a65e3fde4a4b488a6e1697ac41f08a7542))
* add gamepad connection state subscription ([5910337](https://github.com/Nika0000/borealis/commit/5910337be337a1c191001791c166a54ec9811964))
* add glfw support metal ([16b71c6](https://github.com/Nika0000/borealis/commit/16b71c64f5ded5a27bfadc9dbf8f8c3164635ec4))
* add HDR support for D3D11 driver ([f55515d](https://github.com/Nika0000/borealis/commit/f55515d4c9d7f5472dd7f95beb9380bb0251df12))
* add metal backend on apple platform ([e2196f4](https://github.com/Nika0000/borealis/commit/e2196f40416b0ff1374dc939f37367f60d7bfb02))
* add per-corner radius support ([d8f090a](https://github.com/Nika0000/borealis/commit/d8f090ae8a0b5956aba9ed91365f4d5b313465eb))
* add post frame callback ([e757193](https://github.com/Nika0000/borealis/commit/e757193f6f445ac5dfc98a3d47a2e59ac0e645e0))
* add setMuted/isMuted to AudioPlayer to allow suppress UI sounds ([5856213](https://github.com/Nika0000/borealis/commit/585621305a4bf9872d4670b6130d0b04b3126560))
* add support for building wasm with emscripten ([476b73e](https://github.com/Nika0000/borealis/commit/476b73e15c2c93ef8ff8c7827621b1e3b94ca613))
* add support for custom notification display duration ([3a3fee4](https://github.com/Nika0000/borealis/commit/3a3fee4edb1a581c4733d562e90a71dc35964ff9))
* add support for text max line count in label view ([0729a75](https://github.com/Nika0000/borealis/commit/0729a751609f33430f69ba2bdf4e912656ae90d9))
* add support to use custom notification manager ([7811b26](https://github.com/Nika0000/borealis/commit/7811b26165afd055075d69c1c7bd2b1a90dc355d))
* add winrt dpi get ([61699e8](https://github.com/Nika0000/borealis/commit/61699e8c5bcda0ea405fc0643c8c99118bc3bfb2))
* add xmake example option ([7ae501d](https://github.com/Nika0000/borealis/commit/7ae501d7fb63c358a497cf8a4a49802c41ec5a28))
* add xmake script ([cda5e7b](https://github.com/Nika0000/borealis/commit/cda5e7bc05a488997b026e82e70c0881e073220b))
* add XML schema for Borealis UI definitions ([b5bdad1](https://github.com/Nika0000/borealis/commit/b5bdad18e146ea21fc0ec8463577235cd06b77cd))
* add zeromake/nanovg pkg ([7b0bf15](https://github.com/Nika0000/borealis/commit/7b0bf15dcac0c71d4c78268154f48c0d19840ae7))
* borealis add unity_build support ([#65](https://github.com/Nika0000/borealis/issues/65)) ([663a640](https://github.com/Nika0000/borealis/commit/663a640f6f38045cc4080d100ad58feacab26e46))
* build Android project without generating libromfs manually ([b8a5701](https://github.com/Nika0000/borealis/commit/b8a57017fda7bd76f230c87fd8c319aaa09bf8e8))
* custom exit dialog ([5f27ca7](https://github.com/Nika0000/borealis/commit/5f27ca796769c0c0cef2d434f5083a11bbcbc7a8))
* d3d11 sdl ([5a36d11](https://github.com/Nika0000/borealis/commit/5a36d11c64c1d25f54ab08465621301fbdc24c93))
* del extern lib ([09628ed](https://github.com/Nika0000/borealis/commit/09628edc33d371294fd64e9cb0e2ffba52d74f1e))
* del extern tweeny fmt ([780d622](https://github.com/Nika0000/borealis/commit/780d6224ea3ae09320825f30ef1bc025ff1e73d5))
* del extern yoga lib ([aa2d6d6](https://github.com/Nika0000/borealis/commit/aa2d6d64b6f9ad015082f6890d22fc48994618d1))
* del windows size dpi scale ([09165c3](https://github.com/Nika0000/borealis/commit/09165c31998d4ae251c1404d7d2f52e695eba23c))
* disable ALT+Enter fullscreen toggle on Windows ([ff4e29a](https://github.com/Nika0000/borealis/commit/ff4e29a55a716a57a6b1732be57e39855589cf3e))
* free cached entries whoese reference count is zero ([f97931f](https://github.com/Nika0000/borealis/commit/f97931f0a4c88d71ba9fdab40b3563d43e2ceb0a))
* get connected gamepad info with features ([b40a89b](https://github.com/Nika0000/borealis/commit/b40a89b13e57c4b7aa9045a1d12b79cc6963abfb))
* get controller name by type ([1991235](https://github.com/Nika0000/borealis/commit/19912356646309b745501df43b91495a99726dc7))
* glfw d3d11 scaleFactor init ([45b6dc7](https://github.com/Nika0000/borealis/commit/45b6dc74e01866b03453ae50acbcbca7df5846b5))
* hide ios home indicator ([9edbbb2](https://github.com/Nika0000/borealis/commit/9edbbb2a0d6338217a236f539ecd7dad9a6d5948))
* load and play custom audio ([d79bc37](https://github.com/Nika0000/borealis/commit/d79bc37dc3645d193468dec496db44d409eefa07))
* refactoring driver code dir ([7171e7f](https://github.com/Nika0000/borealis/commit/7171e7fc36d31ffb346ed2f84a81c9627895ac71))
* replace sleep-based FPS limiter with spin-wait frame pacing ([bdda06d](https://github.com/Nika0000/borealis/commit/bdda06ddfa10ef9499ca975635cf2acca8e01bf2))
* rumble device if no controller is connected ([ae597d1](https://github.com/Nika0000/borealis/commit/ae597d1ef324caafb89161d4d0e997e0afa57333))
* sdl add d3d11 support(not test) ([63f3a95](https://github.com/Nika0000/borealis/commit/63f3a957e85441ca8f9cfe4a872e1f11245a3f23))
* sdl2 support dx11 ([a36e82d](https://github.com/Nika0000/borealis/commit/a36e82d2b12aee865f1958bc83b7ddf87165ef46))
* SDLImeManager support ([#25](https://github.com/Nika0000/borealis/issues/25)) ([dc42931](https://github.com/Nika0000/borealis/commit/dc429312914ec4855046ff5f92acaf78002ed32c))
* spatial focus navigation for wrapped layouts ([b39b255](https://github.com/Nika0000/borealis/commit/b39b255015115ad35e3828c1659f8002f472389e))
* support windows msvc ([16a7d03](https://github.com/Nika0000/borealis/commit/16a7d03e0a8303bbf9fbfbc32d3e4b61284df369))
* use cppwinrt ([fffaaed](https://github.com/Nika0000/borealis/commit/fffaaedf842430197c8e701f62ca99f166fbc223))
* use nanovg as a submodule ([52bee08](https://github.com/Nika0000/borealis/commit/52bee080ff872548e7ae905c337f66e3e8230d55))
* use tweeny as submodule ([c010fca](https://github.com/Nika0000/borealis/commit/c010fca260cd3bc58aac3cc609acd7d61f4fe0f6))
* use yoga as submodule ([1282f1b](https://github.com/Nika0000/borealis/commit/1282f1b284bd2839b172147893af6902164906bf))
* uwp support ([e609e56](https://github.com/Nika0000/borealis/commit/e609e56c195630b1af3ee25ab8299436e548c23d))
* VideoContext support windowState ([c307b87](https://github.com/Nika0000/borealis/commit/c307b8770c34b7fb1f6d0538947c6b6c67cc9fb1))
* windows opengl support ([0a9d2c1](https://github.com/Nika0000/borealis/commit/0a9d2c1f5ffc673dc65ddf7d1a2537b5614b472e))
* windows support d3d11 driver ([01a497b](https://github.com/Nika0000/borealis/commit/01a497bcb2f38e0bc07fdd85634ddfd46f0791e2))
* winrt use xmake build ([d7d4062](https://github.com/Nika0000/borealis/commit/d7d4062f90039c0309676de3beee29d22bbb1351))
* 支持 uwp ([ad14a41](https://github.com/Nika0000/borealis/commit/ad14a41b691e8d1d93696e92fd6f1c0918ffa054))


### Bug Fixes

* add missing gradle-wrapper.jar ([62f97a4](https://github.com/Nika0000/borealis/commit/62f97a4e3ac7ee4d266dd56cf33b01e62b4cd48d))
* add pdb ([6429bed](https://github.com/Nika0000/borealis/commit/6429bed020909beab22753dbbebab983819d8e51))
* add unity_build options ([7f8cfee](https://github.com/Nika0000/borealis/commit/7f8cfee0a2c399408af7258f9f179a8f0aecfbd0))
* adds switch defines for libretro ([#52](https://github.com/Nika0000/borealis/issues/52)) ([e1e7c17](https://github.com/Nika0000/borealis/commit/e1e7c17510c54125bb6a42c658a161857366059a))
* application might segfault at exit ([#55](https://github.com/Nika0000/borealis/issues/55)) ([d4ba5dd](https://github.com/Nika0000/borealis/commit/d4ba5dd0a72ec06f1056c881a87d2d5a32da0c91))
* async use wait_for ([c96a35c](https://github.com/Nika0000/borealis/commit/c96a35c20fcd2167f9c9fb08539d2b75e22e729f))
* avoid updateTickings crash by deferring deletion ([eba7620](https://github.com/Nika0000/borealis/commit/eba76203d60f7c996d08b29564813a2901755ea3))
* button text wrapping ([f92ae74](https://github.com/Nika0000/borealis/commit/f92ae745ad6399ad506d5c0a8f6d330d5b4dc37a))
* c++20 fmt consteval error ([42ab5ab](https://github.com/Nika0000/borealis/commit/42ab5abfbb37ede0595c4b179a6a77661f1108f3))
* clip highlight rendering to clipped ancestor bounds ([ba2ba80](https://github.com/Nika0000/borealis/commit/ba2ba80b27f9e53ef3a20b6b34ef57cce00e8766))
* correct SDL input/IME scaling for Metal backend ([c94b535](https://github.com/Nika0000/borealis/commit/c94b535139a489bfc7c7479222fd0f91bf1fb216))
* crashes when replacing activity ([a48dc55](https://github.com/Nika0000/borealis/commit/a48dc554ad8a6d1e5021ba3ed57837f4ef977622))
* d3d11 SwapEffect support ([4245b9f](https://github.com/Nika0000/borealis/commit/4245b9ff27dc6d71ee6ce8107997c62c95622242))
* d3d11 use unity_group ([174a8e2](https://github.com/Nika0000/borealis/commit/174a8e25877b2528b5dfe79c738b1323df92ca9c))
* debug layer floods logs with no reason ([1a08da3](https://github.com/Nika0000/borealis/commit/1a08da326140559421b286e844f9df0639e8f4dd))
* define move global ([6707d8a](https://github.com/Nika0000/borealis/commit/6707d8a5f942fcaa358ab6c644370f36c765619c))
* del swap chain desc prop ([af3dfb7](https://github.com/Nika0000/borealis/commit/af3dfb7ad4e87b4c8346f38fb85bae1bfcb84e5c))
* demo use set_rundir ([b57889a](https://github.com/Nika0000/borealis/commit/b57889ac69830482d4a6fe83bb1c05427d418edb))
* drive Android render loop via AChoreographer for correct vsync pacing ([29f5dbf](https://github.com/Nika0000/borealis/commit/29f5dbfb9645130736ae8f2550eababae4b0d728))
* event fire twice selection ([#24](https://github.com/Nika0000/borealis/issues/24)) ([faab357](https://github.com/Nika0000/borealis/commit/faab3575e9b4b7db3ce7b9cc1b00e90de7348268))
* glad only driver == opengl require ([fe8b3a5](https://github.com/Nika0000/borealis/commit/fe8b3a50b0cde683708f95367ead4ee12b9ae347))
* glfw load gamepad mappings from memory ([6c1828e](https://github.com/Nika0000/borealis/commit/6c1828e66f3ff7ecff3817c1520883c6d0357cfe))
* inline winrt cpp to lib ([c5f8f19](https://github.com/Nika0000/borealis/commit/c5f8f198695870f39351d9bdc8fe5bcd1fecb508))
* link winmm for GCC 16 compatibility in GLFW video context ([1a8582e](https://github.com/Nika0000/borealis/commit/1a8582e8f062c67f43051fbb60ac6623287f9c51))
* llvm mingw dll static link ([ad9ab1f](https://github.com/Nika0000/borealis/commit/ad9ab1f7bcbe96346d4b83dc55a46004e3b3d86f))
* load window icon failed on windows using libromfs ([e6eb415](https://github.com/Nika0000/borealis/commit/e6eb415820628b7cb5553111da53a8a19e19e78c))
* macOS fullscreen mode ([6b224b9](https://github.com/Nika0000/borealis/commit/6b224b9e41c9f7b4a4978a6d488a3c52b50dcd71))
* macosx mingw glfw gl init error ([49be3f7](https://github.com/Nika0000/borealis/commit/49be3f7abb48c7469e1483d6068ff91bf48c5376))
* mingw winver ([75fa80c](https://github.com/Nika0000/borealis/commit/75fa80ca7c9a2d0c2360e9c18c2993fcfa0b2726))
* msvc not support dynamic array ([6e7789f](https://github.com/Nika0000/borealis/commit/6e7789f864b1a7b88a47f6b0050f548faa8f20b4))
* nanovg mv to xrepo ([b70f6aa](https://github.com/Nika0000/borealis/commit/b70f6aa9848f3a6e76050e93bdf03ecc2c36a841))
* No shaking animation when switching to an empty focus view ([698ed9f](https://github.com/Nika0000/borealis/commit/698ed9fae8accce5ed16ba0995950f8bce0c0c2f))
* nx fmt del ([9447faa](https://github.com/Nika0000/borealis/commit/9447faae35f34d1bb1b7d1788e95fbfd62879c5e))
* only msvc use c++20 ([d0af869](https://github.com/Nika0000/borealis/commit/d0af86952c209a96ad5da5d573eb88976554ca4d))
* pen gesture when using pen as mouse input ([4c02fce](https://github.com/Nika0000/borealis/commit/4c02fce710937e45347ed2d55e2b7a8f9a8a5d50))
* prevent activity deduplication ([684a1d3](https://github.com/Nika0000/borealis/commit/684a1d371ffbb9b96b2541fe34fd10f85dabe00f))
* pthread_t is int ([6f8bc41](https://github.com/Nika0000/borealis/commit/6f8bc416985c67fde1351aa23c15d890b52cb127))
* RecyclerDataSource memory leak ([#57](https://github.com/Nika0000/borealis/issues/57)) ([821a53e](https://github.com/Nika0000/borealis/commit/821a53eafd49f93e40932c16f0a0ff7b49e09f4c))
* replace spin-wait with hybrid sleep+spin frame pacing ([6ca4cb8](https://github.com/Nika0000/borealis/commit/6ca4cb809c1c1c8b96aba573cf0707fedbaff64c))
* reset library/lib/extern ([c41d292](https://github.com/Nika0000/borealis/commit/c41d292bde65ff545c517de686774f96e77d0816))
* rm useless code ([fc32317](https://github.com/Nika0000/borealis/commit/fc32317d5f263cf9a89668967ba1026b4a3f773c))
* rubber-band drag, fling, and snap-back for ScrollingFrame/HScrollingFrame ([e53d2db](https://github.com/Nika0000/borealis/commit/e53d2db4dcffc47204153870a4b5610270b379ba))
* sdl fullscreen ([b9432c3](https://github.com/Nika0000/borealis/commit/b9432c37e8dcf2a969fb63af58ea2daa8ac3d5e3))
* sdl fullscreen mode ([b503a9b](https://github.com/Nika0000/borealis/commit/b503a9b3f29d81403223a8f61b772a877659877e))
* sdl init is fullscreen ([0f31ea8](https://github.com/Nika0000/borealis/commit/0f31ea8c469c9fb937090fb8ee6d643e62c57a40))
* size scale ([d938c8c](https://github.com/Nika0000/borealis/commit/d938c8c5c922d72f80dfe9bf96cea2fccedf4bc4))
* skip focus traversal for invisible or gone boxes ([b652eb7](https://github.com/Nika0000/borealis/commit/b652eb7d78255478dd03210cbbf94bc3e4a4fdc6))
* stop drawing click animation when hideClickAnimation is set ([d66a9f1](https://github.com/Nika0000/borealis/commit/d66a9f158a6cd6d495737fbdf00c0100f1dc3fba))
* sync fullscreen mode with Borealis on SDL platform ([e7e6b2f](https://github.com/Nika0000/borealis/commit/e7e6b2f8a5c9c0236728c59ad7138b292f273d6d))
* try d3d11 winrt SwapEffect ([6055d86](https://github.com/Nika0000/borealis/commit/6055d8643c71520645499b7df02e3d58404e5aa3))
* upgrade Android SDL project files to match supported SDL version ([b28f801](https://github.com/Nika0000/borealis/commit/b28f801f053c7983aab465f45c67a3704346c6a4))
* use vs sdk version ([63332da](https://github.com/Nika0000/borealis/commit/63332dae15d35eea6280c538a00d111dff76f1c9))
* uwp open url ([#9](https://github.com/Nika0000/borealis/issues/9)) ([e5d34b9](https://github.com/Nika0000/borealis/commit/e5d34b958c323ad166e63a6b34a4f06fc5cd7abd))
* warning about deprecated literal operator in Android NDK r29 ([4896b13](https://github.com/Nika0000/borealis/commit/4896b139bb157c9d04f645ec4f3239430b2d0d37))
* WIN_IsWindows8OrGreater ([6baafdb](https://github.com/Nika0000/borealis/commit/6baafdbfa266a0295b6ef07516f56de779e94d94))
* windows10 not has d3d12_2 ([0fddc60](https://github.com/Nika0000/borealis/commit/0fddc601734dddeb52dc21b40e5e752a37c1e1e2))
* winrt dpi changed ([3668e52](https://github.com/Nika0000/borealis/commit/3668e5274373aae27e2348e549cbd479fabf16ce))
* xmake repo sdl include path ([25760f1](https://github.com/Nika0000/borealis/commit/25760f16f892f5ea7f3eedddd015a110a3758420))
* yoga inline ([2909828](https://github.com/Nika0000/borealis/commit/29098281ca0850ff8ac17957df3ddfddfd52e499))
* yoga nan error ([9a60a0e](https://github.com/Nika0000/borealis/commit/9a60a0e8b70d219a2f0f5bf3cd1db354c49fa82c))
