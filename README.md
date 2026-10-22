# Omni MC для OBS Studio

**Русский** · [English](#omni-mc-for-obs-studio)

Версия плагина: **v1.4.0**. Платформа: **Windows x64**. Проверенная версия OBS Studio: **32.1.0**.

Omni MC — музыкальный плеер для OBS Studio. Плагин получает сведения о текущем треке из медиасессий Windows и выводит их в настраиваемый браузерный источник.

## Демонстрация шаблонов

<p align="center">
  <a href="https://www.youtube.com/watch?v=q8dy48X8Nd8">
    <img src="https://img.youtube.com/vi/q8dy48X8Nd8/maxresdefault.jpg" alt="Демонстрация шаблонов Omni MC" width="800">
  </a>
</p>
<p align="center">
  <a href="https://www.youtube.com/watch?v=q8dy48X8Nd8"><strong>▶ Смотреть демонстрацию шаблонов Omni MC на YouTube</strong></a>
</p>

## Установка

1. Закройте OBS Studio.
2. Скачайте архив для Windows x64 на странице [Releases](https://github.com/xKAPITOSHKAx/Omni-MC/releases).
3. Распакуйте архив.
4. Установите плагин в папку `C:\ProgramData\obs-studio\plugins\omni-mc\bin\64bit`.
5. Запустите OBS Studio. Панель плагина появится в меню **Док-панели → Omni MC**.

Для портативной версии OBS поместите `omni-mc.dll` в `<папка OBS>\obs-plugins\64bit`.

## Первый запуск

1. Откройте панель через меню **Док-панели → Omni MC**.
2. Запустите музыку и выберите приложение в списке либо оставьте автоматический выбор.
3. Настройте внешний вид и нажмите **Добавить плеер в источник**.

<details>
<summary><strong>Полный список возможностей плагина</strong></summary>

- Панель управления **Omni MC** в меню док-панелей OBS и локальная страница настроек.
- Получение названия трека, исполнителя, обложки, длительности, позиции и состояния воспроизведения через Windows System Media Transport Controls.
- Автоматический выбор активной медиасессии, ручной выбор приложения или полное отключение выбора.
- Управление воспроизведением: предыдущий трек, воспроизведение, пауза и следующий трек.
- Перемотка мышью или клавиатурой с помощью полосы прогресса.
- Плавное обновление времени и прогресса между ответами медиасессии с учётом скорости воспроизведения.
- Дополнительное получение позиции и длительности из интерфейса приложения Яндекс Музыка, если медиасессия не передаёт их.
- Управление громкостью выбранного приложения и отключение его звука, если приложение поддерживает такое управление.
- Восемнадцать шаблонов плеера: По умолчанию, Компактный, Обложка, Винил, Стекло, Постер, Кино, Дуэт, Призма, Кассета, Орбита, Затмение, Неон, Вектор, Импульс, Космос, Гаргантюа и Ритм.
- Индивидуальные анимации шаблонов: вращение винила и кассеты, движение тонарма по прогрессу, орбитальные элементы, неоновые эффекты, звёздное поле, аккреционный диск и другие визуальные реакции на воспроизведение и смену обложки.
- Шаблон **Ритм** с выбором аудиоисточника OBS и живым 21‑полосным эквалайзером, реагирующим на звук.
- Показ или скрытие обложки, изменение её размера и автоматическая повторная загрузка, если приложение не сразу передало изображение.
- Показ или скрытие кнопок управления в поддерживаемых шаблонах.
- Настройка ширины и высоты виджета, адаптация размера к содержимому и автоматическое изменение размера браузерного источника.
- Настройка скругления углов, прозрачности, яркости, цвета подложки и цвета полосы прогресса.
- Предварительный просмотр результата в панели настроек и сброс оформления к исходным значениям.
- Автоматический подбор цветов плеера по обложке трека, включая цвет тени.
- Выбор любого установленного системного шрифта, его начертания, размера названия и размера исполнителя.
- Показ или скрытие прогресса и времени трека, а также настройка толщины полосы.
- Четыре режима прогресса: без анимации, плавный возврат, рассыпание и фитиль.
- Прозрачный фон, фон из обложки трека либо выбранной сцены или источника OBS.
- Размытие области под плеером через Composite Blur с настройкой силы и учётом положения и масштаба источника, включая источники внутри групп и вложенных сцен.
- Ограничение фона формой карточки или отдельных панелей шаблона; плеер исключается из захватываемого фона, чтобы не возникала обратная связь.
- Внешняя и внутренняя тень через Stroke Glow Shadow с выбором типа размытия, заполнения, учёта границ, размера, интенсивности, угла, расстояния, цвета или источника заливки.
- Автопоказ плеера после смены трека на время от 1 до 30 секунд.
- Скрытие плеера и его фонового эффекта при паузе, остановке или отсутствии музыки.
- Синхронные анимации плеера и фона: затухание, слева, справа, сверху и снизу.
- Тестовый показ плеера на пять секунд без смены трека.
- Отдельный профиль и отдельный браузерный источник для каждой сцены OBS с автоматическим переключением настроек.
- Автоматическое восстановление связи источника со сценой и исправление его размеров.
- Перенос старого общего источника в отдельные источники сцен с сохранением положения, масштаба, обрезки, порядка и видимости.
- Автоматическое сохранение настроек; состояние свёрнутых разделов, компактный режим панели и цвет её интерфейса также запоминаются.
- Русский и английский интерфейс с автоматическим выбором языка по языку OBS Studio.
- Проверка новых версий через GitHub Releases при запуске OBS. Для отдельной версии уведомление можно отключить.
- Весь интерфейс встроен в DLL и работает локально без внешнего сайта. Сервер доступен только по адресу `127.0.0.1:17843` во время работы OBS.

</details>

## Совместимость и дополнительные плагины

Музыкальное приложение должно передавать медиасессию Windows. Объём доступных данных и команд зависит от самого приложения. Omni MC не распознаёт треки по звуку и не передаёт звук в OBS — для этого используйте захват звука приложения или звук рабочего стола.

Для размытия требуется [Composite Blur](https://obsproject.com/forum/resources/composite-blur.1780/), а для теней — [Stroke Glow Shadow](https://obsproject.com/forum/resources/stroke-glow-shadow.1800/). Остальные функции плеера могут использоваться без этих эффектов.

Настройки сцен сохраняются в `%APPDATA%\obs-studio\plugin_config\omni-mc\scenes`. При переключении сцен загружается соответствующий профиль.

## Исходный код

Нативный код находится в `src`, интерфейс — в `web`. Файл `omni-mc.rc` встраивает HTML, CSS и JavaScript внутрь DLL. Проект использует C++17, Windows SDK и C++/WinRT. Исходные файлы сохранены в UTF-8 без BOM.

## Лицензия

Проект распространяется по лицензии GPL-2.0-or-later. Полный текст находится в файле [LICENSE](LICENSE).

---

# Omni MC for OBS Studio

[Русский](#omni-mc-для-obs-studio) · **English**

Plugin version: **v1.4.0**. Platform: **Windows x64**. Tested with OBS Studio **32.1.0**.

Omni MC is a music player for OBS Studio. It reads the current track from Windows media sessions and displays it in a customizable browser source.

## Template showcase

<p align="center">
  <a href="https://www.youtube.com/watch?v=q8dy48X8Nd8">
    <img src="https://img.youtube.com/vi/q8dy48X8Nd8/maxresdefault.jpg" alt="Omni MC template showcase" width="800">
  </a>
</p>
<p align="center">
  <a href="https://www.youtube.com/watch?v=q8dy48X8Nd8"><strong>▶ Watch the Omni MC template showcase on YouTube</strong></a>
</p>

## Installation

1. Close OBS Studio.
2. Download the Windows x64 archive from the [Releases](https://github.com/xKAPITOSHKAx/Omni-MC/releases) page.
3. Extract the archive.
4. Install the plugin into `C:\ProgramData\obs-studio\plugins\omni-mc\bin\64bit`.
5. Start OBS Studio. The plugin dock will appear under **Docks → Omni MC**.

For a portable OBS installation, place `omni-mc.dll` in `<OBS folder>\obs-plugins\64bit`.

## First launch

1. Open the dock through **Docks → Omni MC**.
2. Start playing music and select an app, or keep automatic selection enabled.
3. Customize the appearance and click **Add player to source**.

<details>
<summary><strong>Full plugin feature list</strong></summary>

- An **Omni MC** control panel in the OBS **Docks** menu and a local settings page.
- Reads the track title, artist, cover art, duration, position, and playback state through Windows System Media Transport Controls.
- Automatic active-session selection, manual app selection, or no selected session.
- Playback controls for previous track, play, pause, and next track.
- Mouse and keyboard seeking through the progress bar.
- Smooth time and progress updates between media-session responses while accounting for the playback rate.
- An additional Yandex Music interface fallback for position and duration when its media session does not provide them.
- Volume and mute control for the selected app when supported.
- Eighteen player templates: Default, Compact, Cover, Vinyl, Glass, Poster, Cinematic, Duo, Prism, Cassette, Orbit, Eclipse, Neon, Vector, Pulse, Cosmos, Gargantua, and Rhythm.
- Template-specific animations include spinning vinyl and cassette reels, a progress-driven tonearm, orbiting elements, neon effects, a star field, an accretion disk, and other reactions to playback and cover changes.
- The **Rhythm** template with a selectable OBS audio source and a live 21-band equalizer driven by its audio.
- Cover visibility and size controls, with automatic retries when an app does not provide its artwork immediately.
- Optional playback buttons in supported templates.
- Widget width and height controls, content-aware sizing, and automatic browser-source resizing.
- Controls for corner radius, opacity, brightness, background color, and progress color.
- A live preview in the settings dock and an option to reset the appearance to its defaults.
- Automatic player colors based on the track cover, including the shadow color.
- Selection of any installed system font, font weight, title size, and artist size.
- Optional track progress and time display with adjustable bar thickness.
- Four progress modes: none, smooth return, scatter, and wick.
- Transparent background, track cover background, or any selected OBS scene or source.
- Composite Blur integration with adjustable strength that accounts for source position and scale, including sources inside groups and nested scenes.
- Background clipping to the player card or separate template panels; the player is excluded from its captured background to prevent feedback.
- Outer and inner shadows through Stroke Glow Shadow with blur type, fill, source-boundary handling, size, intensity, angle, distance, color, and fill-source controls.
- Automatic player display for 1 to 30 seconds after a track changes.
- Hides the player and its background effect while paused, stopped, or without music.
- Synchronized player and background animations: fade, from left, right, top, or bottom.
- A five-second test preview without changing tracks.
- An independent profile and browser source for every OBS scene, with automatic settings switching.
- Automatic repair of scene bindings and browser-source dimensions.
- Migration from the old shared source to per-scene sources while preserving position, scale, crop, order, and visibility.
- Automatic settings saving; collapsed sections, compact dock mode, and the dock accent color are remembered as well.
- Russian and English interfaces selected automatically from the OBS Studio language.
- Checks GitHub Releases for updates when OBS starts. Notifications can be disabled for a specific version.
- The entire interface is embedded in the DLL and works locally without an external website. The server is available only at `127.0.0.1:17843` while OBS is running.

</details>

## Compatibility and optional plugins

The music app must expose a Windows media session. Available metadata and controls depend on the app. Omni MC does not identify tracks from audio and does not route audio into OBS; use Application Audio Capture or desktop audio for that.

[Composite Blur](https://obsproject.com/forum/resources/composite-blur.1780/) is required for background blur, and [Stroke Glow Shadow](https://obsproject.com/forum/resources/stroke-glow-shadow.1800/) is required for shadows. Other player features work without these effects.

Scene profiles are stored in `%APPDATA%\obs-studio\plugin_config\omni-mc\scenes`. The corresponding profile loads when the active scene changes.

## Source code

Native code is located in `src`, and the interface is located in `web`. The `omni-mc.rc` file embeds the HTML, CSS, and JavaScript into the DLL. The project uses C++17, the Windows SDK, and C++/WinRT. Source files are stored as UTF-8 without BOM.

## License

This project is licensed under GPL-2.0-or-later. See [LICENSE](LICENSE) for the full text.
