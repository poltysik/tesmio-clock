# Tesmio Clock 1.0.0 — русская инструкция

Tesmio Clock добавляет в верхнюю панель **Workers & Resources: Soviet Republic**
понятные цифровые часы. Это дополнение для
[Calendar Synchronizer](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468):
часы считывают положение реального игрового цикла дня и ночи, а не запускают
собственный независимый таймер.

> **Важно: это не самостоятельный и не полноценный мод синхронизации времени.**
> Tesmio Clock является только интерфейсным дополнением к
> [Calendar Synchronizer от Tesmio / MaxLegend](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468).
> Сначала обязательно подпишитесь и установите оригинальный мод автора. Вся
> логика замедления календаря и синхронизации дня и ночи принадлежит Calendar
> Synchronizer; наш проект добавляет отображение часов и проверенную калибровку.

Время меняется с шагом 30 минут. Калибровка соответствует освещению игры:
примерно в 05:00 начинается рассвет, около 07:00 становится светло, около 20:00
начинается закат, а около 22:00 наступает ночь. Дата меняется ровно в 00:00.

## Возможности

- Два отдельно проверенных варианта:
  - `16:00` — 24-часовой формат;
  - `4:00 PM` — 12-часовой формат AM/PM.
- Обновление каждые полчаса без мелькающего поминутного счётчика.
- Время и дата аккуратно размещены в одном расширенном поле штатного интерфейса.
- Убрана ненужная всплывающая полоса ванильного календаря.
- Значения валют и дата выровнены по вертикали в верхней панели.
- Вместе с Calendar Synchronizer один календарный день соответствует одному
  циклу дня и ночи.
- Мод не изменяет сохранения и не редактирует `SOVIET64.exe` на диске.

## Требования

- Workers & Resources: Soviet Republic `1.1.1.7`, 64-битная DX11-версия.
- [TesmioLoader](https://steamcommunity.com/sharedfiles/filedetails/?id=3773169177)
  API 3 / launcher `b0.3.4`.
- Установленный
  [Calendar Synchronizer](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468)
  (`daynight.dll` и `daynight.ini` в папке плагинов TesmioLoader).
- Запуск игры через `tesmiolauncher.exe`.

После обновления игры или TesmioLoader может потребоваться новая версия мода.

## Автоматическая установка

1. Сначала установите TesmioLoader и Calendar Synchronizer.
2. Скачайте и распакуйте `TesmioClock-1.0.0.zip` либо откройте папку предмета Workshop.
3. Запустите `INSTALL-TESMIO-CLOCK.bat`.
4. Выберите:
   - `1` — 24-часовой формат;
   - `2` — формат AM/PM.
5. Запустите `tesmiolauncher.exe`, включите `TesmioClock.dll` и нажмите **Launch**.

Установщик сам найдёт игру, скопирует только один вариант часов, отключит старый
тестовый файл `GameClock.dll`, проверит установленную DLL и применит проверенную
калибровку Calendar Synchronizer. Перед первым изменением `daynight.ini`
создаётся резервная копия `daynight.ini.tesmioclock-backup`.

Чтобы позже сменить формат, просто снова запустите установщик. Выбор хранится в
файле `clock-format.ini`. Его также можно изменить вручную:

```ini
[clock]
format = 24-hour
```

или:

```ini
[clock]
format = 12-hour-am-pm
```

## Ручная установка

Скопируйте выбранный файл:

```text
variants\24-hour\TesmioClock.dll
```

или:

```text
variants\12-hour-am-pm\TesmioClock.dll
```

в папку:

```text
SovietRepublic\tesmioloader\build\plugins\TesmioClock.dll
```

Одновременно должен находиться только один вариант. Старый `GameClock.dll`
необходимо удалить или отключить. Проверенные параметры Calendar Synchronizer:

```ini
enabled = 1
cycle_days = 1
day_scale = auto
offset = 0.5825
fade = 1
probe = 0
```

## Если мод не работает

- Убедитесь, что игра запущена через `tesmiolauncher.exe`.
- Проверьте, что включены `TesmioClock.dll` и `daynight.dll`.
- Не включайте одновременно `GameClock.dll` и `TesmioClock.dll`.
- Проверьте `tesmioloader.log` по словам `Tesmio Clock`, `missing engine import`
  и `install failed`.
- После смены формата полностью перезапустите игру и лаунчер.

## Лицензия

GPL-3.0-or-later. ABI TesmioLoader основан на GPL-3.0-проекте TesmioLoader
автора MaxLegend.

## Благодарность

Calendar Synchronizer и TesmioLoader созданы **Tesmio / MaxLegend**. Спасибо
автору оригинального синхронизатора, без которого это дополнение не работает.
