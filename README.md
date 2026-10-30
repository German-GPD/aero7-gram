<p align="center">
  <img src="assets/aero7-icon.svg" width="104" alt="AERO7 GRAM icon">
</p>

<h1 align="center">AERO7 GRAM</h1>
<p align="center"><b>Telegram с настоящим духом Windows 7 Aero.</b></p>
<p align="center">
  <a href="https://github.com/German-GPD/aero7-gram"><img src="https://img.shields.io/badge/platform-Windows%207%20SP1-2879a5?style=for-the-badge&logo=windows&logoColor=white" alt="Windows 7 SP1"></a>
  <a href="https://github.com/German-GPD/aero7-gram/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-4c9b83?style=for-the-badge" alt="MIT License"></a>
  <a href="https://github.com/German-GPD/aero7-gram/releases"><img src="https://img.shields.io/github/v/release/German-GPD/aero7-gram?display_name=tag&style=for-the-badge&color=3986a8" alt="Latest release"></a>
</p>
<p align="center">
  <a href="https://german-gpd.github.io/aero7-gram/">🌐 Сайт проекта</a> ·
  <a href="https://github.com/German-GPD/aero7-gram/releases">📦 Релизы</a> ·
  <a href="docs/BUILDING-WINDOWS7.md">🛠 Сборка</a>
</p>

---

> [!IMPORTANT]
> **Проект находится в ранней разработке.** Тестовые установщики оболочки автоматически публикуются в [GitHub Releases](https://github.com/German-GPD/aero7-gram/releases/tag/test-latest) после успешной сборки. Это не полноценный Telegram-клиент: TDLib пока не включена, авторизация и сетевые функции в этих архивах недоступны. Совместимость с Windows 7 SP1 ещё нужно проверить на реальном ПК или виртуальной машине.

## ✨ Что такое AERO7 GRAM?

AERO7 GRAM — независимый проект нативного Telegram-клиента для **Windows 7 SP1**. Задача — объединить Telegram с визуальным языком Windows 7: Aero Glass, характерными градиентами, системными элементами управления и привычным поведением окон.

Это не просто веб-страница в синей теме: приложение написано на C++ и Qt Widgets, а Telegram-интеграция строится на TDLib.

<p align="center">
  <a href="https://german-gpd.github.io/aero7-gram/">
    <img src="https://img.shields.io/badge/ОТКРЫТЬ-САЙТ%20ПРОЕКТА-237ca5?style=for-the-badge" alt="Открыть сайт проекта">
  </a>
</p>

## 📥 Скачать

| Система | Архитектура | Загрузка |
|:--|:--:|:--|
| Windows 64-bit | **x64** | [Скачать Setup.exe](https://github.com/German-GPD/aero7-gram/releases/download/test-latest/AERO7-GRAM-x64-Setup.exe) |
| Windows 32-bit | **x86** | [Скачать Setup.exe](https://github.com/German-GPD/aero7-gram/releases/download/test-latest/AERO7-GRAM-x86-Setup.exe) |

Скачай установщик нужной архитектуры, запусти его и следуй мастеру. По умолчанию приложение устанавливается в `C:\\Program Files\\German\\G-Code-Team\\Aero7Gram`. Для установки нужны права администратора.

> **Ограничения тестовой сборки:** это только проверка интерфейса и оболочки. TDLib не включена, поэтому вход в Telegram и обмен сообщениями не работают. Сборка на Windows runner не доказывает совместимость с Windows 7 SP1 — это отдельный этап тестирования. Установщик создаёт ярлык в меню «Пуск» и предлагает ярлык на рабочем столе.

## 🧩 Текущее состояние

| Компонент | Состояние |
|:--|:--|
| Нативная оболочка Qt Widgets | ✅ Реализована |
| Заголовок окна и управление окном | ✅ Реализовано |
| Интеграция DWM Aero Glass | ✅ Начальная реализация |
| Настройки названия и акцентной темы | ✅ Реализованы |
| Авторизация через TDLib | 🧪 Начальная реализация |
| Список диалогов и история | 🧪 Начальный путь |
| Отправка текстовых сообщений | 🧪 Начальный путь |
| Контакты и управление группами/каналами | 🕓 В планах |
| Фото, файлы и загрузки | 🕓 В планах |
| Поиск, уведомления и звонки | 🕓 В планах |
| Проверенные сборки Windows 7 x86/x64 | 🕓 В планах |

### Уже реализовано

- Нативное окно на Qt Widgets с Aero-оформлением и управлением размером.
- Навигация по разделам, настройки и сохранение геометрии окна.
- Опциональная интеграция TDLib.
- Начальные состояния авторизации: номер телефона, код, пароль двухэтапной проверки, регистрация и подтверждение email.
- Запрос основного списка чатов, загрузка истории и отправка обычного текста.
- Локальная база TDLib в профиле пользователя; ключ шифрования базы на Windows защищается DPAPI.

> [!NOTE]
> TDLib не включена в репозиторий. Для реального входа требуется отдельно собранная версия TDLib, совместимая с Windows 7 SP1 и архитектурой приложения. Текущий исходный код ещё не прошёл сборку и проверку на Windows 7.

## 🪟 Визуальная идея

<p align="center">
  <img src="assets/aero7-icon.svg" width="120" alt="AERO7 GRAM">
</p>
<p align="center"><i>Aero Glass · Segoe UI · системные градиенты · классические контролы</i></p>

Цель — интерфейс, который ощущается частью Windows 7, а не современным приложением с декоративным синим фоном. Визуальные материалы сайта показывают направление дизайна и не являются скриншотами готового приложения.

## 🔐 Приватность и безопасность

- В проект не закладываются рекламные блоки, скрытые трекеры и собственная аналитика.
- Коды входа и пароль двухэтапной проверки передаются TDLib и не должны сохраняться в настройках приложения.
- База TDLib хранится локально в профиле пользователя.
- На Windows ключ базы защищается DPAPI.
- API ID и API hash Telegram нельзя публиковать в репозитории или передавать другим людям.
- Сторонние зависимости сохраняют собственные лицензии и правила обработки данных.

Обычные облачные чаты Telegram **не зашифрованы сквозным шифрованием по умолчанию**. Для E2E-шифрования предназначены Secret Chats. Подробнее — [FAQ Telegram](https://telegram.org/faq#q-how-secure-is-telegram).

## 🛠 Технологии

- **C++14**
- **Qt 5.15 Widgets / Qt SVG**
- **TDLib** — опциональный Telegram backend
- **CMake**
- **Windows DWM / DPAPI**
- **Visual Studio 2019 toolchain** для целевой сборки

## 🧱 Сборка

Подробные команды для x86 и x64, требования к Qt и TDLib собраны здесь:

➡️ **[BUILDING-WINDOWS7.md](docs/BUILDING-WINDOWS7.md)**

Кратко: нужны Windows 7 SP1-совместимые Qt 5.15, компилятор и зависимости одной архитектуры. Нельзя смешивать x86 и x64 библиотеки. Успешная сборка на Windows 10/11 не доказывает совместимость с Windows 7.

## 🗺 Дорожная карта

- [x] Основа нативного приложения
- [x] Начальная авторизация и текстовые чаты через TDLib
- [ ] Стабилизация синхронизации и обработки ошибок
- [ ] Контакты и полноценная навигация по чатам
- [ ] Медиа, файлы и загрузки
- [ ] Поиск, уведомления и настройки аккаунта
- [ ] Сборки x86 и x64
- [ ] Проверка на чистых установках Windows 7 SP1

## 📜 Лицензия и статус

Оригинальные материалы проекта распространяются по лицензии **MIT**. Сторонние библиотеки и ресурсы регулируются собственными лицензиями.

AERO7 GRAM — независимый неофициальный проект, не связанный с Telegram или Microsoft. Telegram, Windows и Aero — товарные знаки соответствующих владельцев.

---

<p align="center">
  <b>Сделано для тех, кто всё ещё любит Windows 7.</b><br>
  <a href="https://github.com/German-GPD/aero7-gram">GitHub</a> ·
  <a href="https://german-gpd.github.io/aero7-gram/">Сайт</a>
</p>
