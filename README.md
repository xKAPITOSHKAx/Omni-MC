# Omni MC для OBS Studio

**Русский** · [English](#omni-mc-for-obs-studio)

Версия плагина: **v1.3.6**. Платформа: **Windows x64**. Проверенная версия OBS Studio: **32.1.0**.

Omni MC — музыкальный плеер для OBS Studio. Плагин получает сведения о текущем треке из медиасессий Windows и выводит их в настраиваемый браузерный источник.

## Установка и использование

1. Скачайте готовый архив на странице [Releases](https://github.com/xKAPITOSHKAx/Omni-mc/releases).
2. Установите файлы плагина и перезапустите OBS Studio.
3. Откройте панель **Omni MC** или выберите **Сервис → Omni MC — Музыкальный плеер**.
4. Запустите музыку и выберите приложение в списке либо оставьте автоматический выбор.
5. Настройте внешний вид и нажмите **Добавить плеер в источник**.

<details>
<summary><strong>Полный список возможностей плагина</strong></summary>

- Получение названия трека, исполнителя, обложки, длительности, позиции и состояния воспроизведения через Windows System Media Transport Controls.
- Автоматический выбор активной медиасессии или ручной выбор приложения.
- Управление воспроизведением: предыдущий трек, воспроизведение, пауза и следующий трек.
- Перемотка трека с помощью полосы прогресса.
- Управление громкостью выбранного приложения и отключение его звука, если приложение поддерживает такое управление.
- Семнадцать шаблонов плеера: По умолчанию, Компактный, Обложка, Винил, Стекло, Постер, Кино, Дуэт, Призма, Кассета, Орбита, Затмение, Неон, Вектор, Импульс, Космос и Гаргантюа.
- Настройка обложки, размеров виджета, скругления углов, прозрачности, яркости, цветов, шрифта, начертания и размеров текста.
- Автоматический подбор цветов интерфейса по обложке трека.
- Несколько анимаций полосы прогресса: без анимации, плавный возврат, рассыпание и фитиль.
- Прозрачный фон, фон из обложки трека либо выбранной сцены или источника OBS.
- Размытие области под плеером через Composite Blur с учётом положения и масштаба источника в сцене.
- Внешняя и внутренняя тень с настройкой размера, интенсивности, угла, расстояния, цвета и источника заливки через Stroke Glow Shadow.
- Автопоказ плеера после смены трека с настраиваемой длительностью.
- Скрытие плеера, когда музыка не воспроизводится.
- Анимации появления и скрытия: затухание, слева, справа, сверху и снизу.
- Отдельные настройки для каждой сцены OBS. При переключении сцены панель автоматически загружает её профиль.
- Автоматическое изменение размера браузерного источника при выборе другого шаблона.
- Встроенная тестовая кнопка для предварительного просмотра плеера без смены трека.
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

Plugin version: **v1.3.6**. Platform: **Windows x64**. Tested with OBS Studio **32.1.0**.

Omni MC is a music player for OBS Studio. It reads the current track from Windows media sessions and displays it in a customizable browser source.

## Installation and usage

1. Download the ready-to-use archive from the [Releases](https://github.com/xKAPITOSHKAx/Omni-mc/releases) page.
2. Install the plugin files and restart OBS Studio.
3. Open the **Omni MC** dock or select **Tools → Omni MC — Music Player**.
4. Start playing music and select an app, or keep automatic selection enabled.
5. Customize the appearance and click **Add player to source**.

<details>
<summary><strong>Full plugin feature list</strong></summary>

- Reads the track title, artist, cover art, duration, position, and playback state through Windows System Media Transport Controls.
- Automatically selects the active media session or lets you select an app manually.
- Playback controls for previous track, play, pause, and next track.
- Track seeking through the progress bar.
- Volume and mute control for the selected app when supported.
- Seventeen player templates: Default, Compact, Cover, Vinyl, Glass, Poster, Cinematic, Duo, Prism, Cassette, Orbit, Eclipse, Neon, Vector, Pulse, Cosmos, and Gargantua.
- Controls for cover art, widget size, corner radius, opacity, brightness, colors, font family, font weight, and text size.
- Automatic interface colors based on the track cover.
- Multiple progress animations: none, smooth return, scatter, and wick.
- Transparent background, track cover background, or any selected OBS scene or source.
- Composite Blur integration that blurs only the area beneath the player while accounting for its scene position and scale.
- Outer and inner shadows with adjustable size, intensity, angle, distance, color, and fill source through Stroke Glow Shadow.
- Automatic player display after a track change with a configurable duration.
- Option to hide the player when nothing is playing.
- Show and hide animations: fade, from left, right, top, or bottom.
- Independent settings for every OBS scene. The matching profile loads automatically when the scene changes.
- Automatic browser-source resizing when another template is selected.
- Built-in test button for previewing the player without changing tracks.
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
