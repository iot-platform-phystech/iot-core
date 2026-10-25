# iot-core

Core IoT-платформы: пользователи, устройства, измерения и уведомления Telegram.
HTTP-адаптер отвечает за Base64/AES-256-GCM и связь с сенсором, core получает
обычный JSON и хранит данные в PostgreSQL.

## Архитектура

```text
HTTP -> ApiController -> UserService / DeviceService / TelemetryService
                              |                 |
                     SQL repositories     NotificationService
                              |                 |
                         PostgreSQL       HttpRepository -> Telegram

DeviceService -> HttpRepository -> HTTP adapter -> Encoder -> sensor
sensor -> HTTP adapter -> Decoder -> TelemetryService -> PostgreSQL
```

- `controllers/`: маршруты, HTTP-статусы, преобразование запросов и ответов.
- `services/`: валидация, CRUD, команды устройствам, запуск уведомлений.
- `repositories/`: параметризованный SQL и HTTP-транспорт внешних сервисов.
- `main.cc`: создание зависимостей и запуск сервера; SQL и обработчиков здесь нет.
- `migrations/001_schema.sql`: таблицы, индексы, ограничения связей и атомарные
  функции записи пользователей/устройств. Функции преобразуют ожидаемые SQL-конфликты
  в статусы: PostgreSQL-драйвер Drogon 1.9.3 не сохраняет тип исключения.

Адаптер имеет такие же слои и HTTP-repository для core/сенсора; собственной БД
и дублирующих таблиц пользователей/устройств у него нет.

## Запуск обоих сервисов

Каталоги `iot-core` и `iot-http-adapter` должны лежать рядом. В каталоге core:

```bash
cp .env.example .env
openssl rand -base64 32
# Запишите сгенерированный ключ в IOT_AES_KEY_BASE64 в .env.
# Тот же ключ должен быть настроен на сенсоре.
docker compose up --build -d
```

Core доступен на `http://127.0.0.1:8000`, адаптер — `http://127.0.0.1:8080`.
Для режима без шифрования задайте в `.env` `IOT_SENSOR_ALGORITHM=base64`.
В этом режиме ключ не требуется. Base64 не скрывает данные.

PostgreSQL хранится в именованном volume; начальная схема создаётся при первом
запуске пустой БД. Для уже существующего volume примените миграцию явно:

```bash
docker compose exec -T db psql -U iot -d iot_db -v ON_ERROR_STOP=1 \
  < migrations/001_schema.sql
```

Миграция сохраняет имеющиеся измерения и допускает повторный запуск. Для старой
таблицы `sensor_data` она добавляет `received_at`, если столбца нет. Если ваша
существующая схема отличается от исходного проекта, её нужно согласовать до
применения миграции. Новые измерения принимаются только от зарегистрированных
включённых устройств; прежние записи остаются доступными.

## API пользователей и устройств

CRUD доступен и через core, и через адаптер по одинаковым путям:

| Метод | Путь | Назначение |
| --- | --- | --- |
| POST | `/api/v1/users` | Создать пользователя |
| GET | `/api/v1/users?limit=100` | Список пользователей |
| GET / PUT / DELETE | `/api/v1/users/{id}` | Получить / заменить / удалить |
| POST | `/api/v1/devices` | Создать устройство |
| GET | `/api/v1/devices?limit=100` | Список устройств |
| GET / PUT / DELETE | `/api/v1/devices/{id}` | Получить / заменить / удалить |
| POST | `/api/v1/devices/{id}/commands` | Передать команду через core и адаптер |
| POST | `/api/v1/ingest` | Сохранить измерение |
| GET | `/api/v1/data?limit=100` | Последние измерения |
| GET | `/health` (core), `/ping` (адаптер) | Проверка работы HTTP-сервера |

`limit`: 1–1000. Создание возвращает `201`, удаление — `204`, чтение/обновление —
`200`; ошибки валидации — `400`, отсутствующая запись — `404`, конфликт уникальности
или связи — `409`, недоступная БД — `503`, недоступный HTTP-upstream — `502`.
`/health` — проверка процесса, а не готовности БД.

Создание пользователя:

```bash
curl http://127.0.0.1:8080/api/v1/users -H 'Content-Type: application/json' \
  -d '{"name":"Иван","email":"ivan@example.com","telegram_chat_id":"123456789","notifications_enabled":true}'
```

Сохраните возвращённый `id` и используйте его как `user_id`:

```bash
curl http://127.0.0.1:8080/api/v1/devices -H 'Content-Type: application/json' \
  -d '{"sensor_id":"sensor-1","name":"Температура","user_id":1,"enabled":true}'
```

Пользователь: обязательны `name` и уникальный `email` (приводится к нижнему
регистру). `telegram_chat_id` — строка, по умолчанию пустая;
`notifications_enabled` — boolean, по умолчанию `true`.

Устройство: обязательны `sensor_id`, `name`, существующий `user_id`;
`enabled` — boolean, по умолчанию `true`. `sensor_id` уникален и неизменяем:
при `PUT` он должен отсутствовать. Пример обновления устройства:

```bash
curl -X PUT http://127.0.0.1:8080/api/v1/devices/1 \
  -H 'Content-Type: application/json' \
  -d '{"name":"Новая температура","user_id":1,"enabled":false}'
```

`PUT` заменяет редактируемые поля целиком, а не выполняет частичное обновление.
Пропущенные необязательные поля получают значения по умолчанию. Пользователя
нельзя удалить, пока у него есть устройства. Удаление устройства сохраняет
историю измерений. Повторная регистрация того же `sensor_id` продолжит историю
этого идентификатора.

## Измерения и команды

В core на `/api/v1/ingest` передаётся открытый объект:

```json
{"sensor_id":"sensor-1","value":23.5,"unit":"C"}
```

`unit` необязателен (по умолчанию `unknown`). `value` должен быть конечным числом.
Неизвестное или выключенное устройство: `404`, измерение не сохраняется.
В адаптер этот объект передаётся в конверте `algorithm`/`payload`; описание
Base64 и AES-256-GCM находится в README адаптера.

Команда через core или адаптер:

```bash
curl http://127.0.0.1:8080/api/v1/devices/1/commands \
  -H 'Content-Type: application/json' \
  -d '{"command":"set_interval","seconds":10}'
```

Core проверяет устройство, подставляет его `sensor_id` и вызывает
`POST {IOT_ADAPTER_URL}/api/v1/commands`. Адаптер кодирует команду и отправляет её
по `IOT_SENSOR_URL` + `IOT_SENSOR_PATH`. Выключенное устройство: `409`.
Сенсор должен возвращать JSON-подтверждение. Один адаптер настроен на один адрес
сенсора/шлюза; шлюз может маршрутизировать команды по `sensor_id`.

## Telegram

Создайте бота через `@BotFather`, запишите токен в `IOT_TELEGRAM_BOT_TOKEN` в `.env`
и перезапустите core. Пользователь должен открыть чат с ботом и нажать Start.
Укажите его chat ID строкой в `telegram_chat_id` пользователя через POST/PUT.
ID можно получить из `message.chat.id` ответа Bot API `getUpdates` после сообщения
боту (если бот не использует webhook).

После успешного сохранения измерения `NotificationService` отправляет владельцу
сообщение с `sensor_id`, значением и единицей через
[Telegram Bot API sendMessage](https://core.telegram.org/bots/api#sendmessage).
Без токена, без chat ID или при `notifications_enabled=false` отправки нет.
Входящие команды Telegram, polling/webhook и управление устройствами из чата
не реализованы: бот используется для исходящих уведомлений.

Отправка асинхронная, таймаут — 5 секунд. Ошибка HTTP, `ok:false` или недоступность
Telegram журналируются без токена и не отменяют сохранённое измерение. Повторных
попыток и постоянной очереди нет, поэтому доставка best effort. При высокой
частоте измерений нужно добавить агрегацию/очередь с учётом лимитов Telegram.

## Конфигурация core

| Переменная | По умолчанию |
| --- | --- |
| `IOT_CORE_HOST` | `127.0.0.1` (Docker: `0.0.0.0`), порт 8000 |
| `IOT_DATABASE_URL` | `host=127.0.0.1 port=5432 dbname=iot_db user=iot password=iotpass` |
| `IOT_ADAPTER_URL` | `http://127.0.0.1:8080` |
| `IOT_TELEGRAM_BOT_TOKEN` | пусто, отправка выключена |
| `IOT_TELEGRAM_API_URL` | `https://api.telegram.org`; переопределяется для тестовой заглушки |

`IOT_DATABASE_URL` — строка подключения libpq. Конфигурация теперь задаётся
окружением; прежний `config.json` с неподключённым разделом `database` удалён.
Пул БД: 4 подключения, таймаут запросов: 5 секунд.

API рассчитан на доверенную внутреннюю сеть: CRUD не добавляет регистрацию,
пароли, аутентификацию или разграничение доступа. Compose публикует порты только
на loopback хоста. Перед внешним доступом настройте авторизацию и TLS.

## Сборка и проверка

Для локальной сборки нужен Drogon с поддержкой PostgreSQL и C++20:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/dependencies
cmake --build build -j4
```

Сквозной тест использует отдельные временные контейнеры PostgreSQL/core,
локальный бинарник адаптера и заглушки сенсора/Telegram. Нужны Docker Desktop,
Python 3 и свободный порт 8080. Реальный бот и существующие БД не используются.

```bash
docker build -t iot-core-task-test .
python3 tests/integration.py ../iot-http-adapter/build/debug/iot-http-adapter
```

Проверяются CRUD, связи и дубликаты, валидация, команды, оба кодека, запись и
сохранение истории, Telegram-уведомления, отключение уведомлений и ошибка Telegram.
Временные контейнеры и сеть удаляются в `finally`.

Заготовки coroutine-интерфейсов из параллельной ветки сохранены в
`repositories/interfaces/`, вместе с `*DbRepository`, `UserService.h`,
`DeviceService.h` и отдельными User/Device-контроллерами. Они пока содержат
объявления без реализаций и не подключены к серверу; рабочие маршруты
регистрирует `ApiController`.
