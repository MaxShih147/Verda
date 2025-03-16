#if defined(ARDUINO)

#define WIFI_SSID     "..."
#define WIFI_PASSWORD "..."
#define NTP_TIMEZONE  "UTC-8"
#define NTP_SERVER1   "0.pool.ntp.org"
#define NTP_SERVER2   "1.pool.ntp.org"
#define NTP_SERVER3   "2.pool.ntp.org"

#include <WiFi.h>

// 檢查 SNTP 庫是否可用
#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define SNTP_ENABLED 1
#elif __has_include(<sntp.h>)
#include <sntp.h>
#define SNTP_ENABLED 1
#endif

#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

#include <M5CoreS3.h>
#include <lvgl.h>

// **LVGL UI 變數**
static lv_obj_t *time_label;
static lv_obj_t *date_label;

static lv_disp_drv_t disp_drv;
lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 10];

// **更新時間到 LVGL UI**
void update_time_ui() {

    Serial.println("✅ Time updating...");
    static constexpr const char* const week_days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    auto dt = CoreS3.Rtc.getDateTime();

    // **格式化時間**
    char time_str[10];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);

    // **格式化日期**
    char date_str[20];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %s", dt.date.year, dt.date.month, dt.date.date, week_days[dt.date.weekDay]);

    // **更新 LVGL UI**
    lv_label_set_text(time_label, time_str);
    lv_label_set_text(date_label, date_str);
    
    // **強制刷新畫面**
    lv_obj_invalidate(time_label);
    lv_obj_invalidate(date_label);
    lv_refr_now(NULL);  // **強制刷新 LVGL**
}

// **WiFi 連線與 NTP 時間同步**
void sync_time_with_ntp() {
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println("\n✅ WiFi Connected!");

    // **同步 NTP**
    configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

#if SNTP_ENABLED
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println("\n✅ NTP Time Synced!");
#else
    delay(1600);
    struct tm timeInfo;
    while (!getLocalTime(&timeInfo, 1000)) {
        Serial.print('.');
    }
#endif

    // **同步 RTC**
    time_t t = time(nullptr) + 1;
    while (t > time(nullptr));
    CoreS3.Rtc.setDateTime(gmtime(&t));
    Serial.println("✅ RTC Time Updated!");
}

void my_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    Serial.println("✅ Enter the flush callback!");

    // **檢查 color_p 是否為 NULL，避免崩潰**
    if (!color_p) {
        Serial.println("❌ ERROR: color_p is NULL!");
        lv_disp_flush_ready(disp);
        return;
    }

    M5.Lcd.startWrite();
    M5.Lcd.setAddrWindow(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area));
    M5.Lcd.pushColors((uint16_t *)color_p, lv_area_get_width(area) * lv_area_get_height(area), true);
    M5.Lcd.endWrite();
    lv_disp_flush_ready(disp);
}

void setup() {
    Serial.begin(115200);
    CoreS3.begin();

    if (!CoreS3.Rtc.isEnabled()) {
        Serial.println("❌ RTC not found.");
        for (;;) {
            vTaskDelay(500);
        }
    }
    Serial.println("✅ RTC found.");

    // **WiFi 連線與 NTP 時間同步**
    sync_time_with_ntp();

    // **初始化 LVGL**
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 320 * 10);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // **設置 LVGL 畫面**
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    // **顯示大字體時間**
    time_label = lv_label_create(screen);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -30);

    // **顯示日期**
    date_label = lv_label_create(screen);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_44, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 30);

    Serial.println("✅ LVGL UI Initialized!");
}

void loop() {
    update_time_ui();  // **更新時間顯示**
    lv_timer_handler();  // **LVGL 必須定期執行**
    delay(1000);  // **每秒更新一次**
}
