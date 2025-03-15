#include <M5Unified.h>
#include <lvgl.h>

// **棋盤格設定**
#define GRID_SIZE 4   // 棋盤格大小 (4x4)
#define CELL_SIZE 50  // 每個格子的大小
#define BOARD_X 20    // 棋盤格 X 偏移量
#define BOARD_Y 20    // 棋盤格 Y 偏移量

// **按鈕 GPIO 設定**
#define BTN_BLUE  8  // 🔵 藍色按鈕 (GPIO 8)
#define BTN_RED   9  // 🔴 紅色按鈕 (GPIO 9)

// **LVGL 變數**
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 10];
static lv_disp_drv_t disp_drv;
lv_obj_t *grid[GRID_SIZE][GRID_SIZE];  // 棋盤格

// **隨機產生一個方塊**
void create_random_square(lv_color_t color) {
    int x = random(GRID_SIZE);
    int y = random(GRID_SIZE);

    lv_obj_t *square = lv_obj_create(lv_scr_act());  
    lv_obj_set_size(square, CELL_SIZE, CELL_SIZE);
    lv_obj_set_style_bg_color(square, color, LV_PART_MAIN);
    lv_obj_align(square, LV_ALIGN_TOP_LEFT, BOARD_X + x * CELL_SIZE, BOARD_Y + y * CELL_SIZE);
    
    Serial.println("🖥️ UI Updated - Marking screen for redraw...");
    lv_obj_invalidate(square);  // **標記畫面需要更新**

    Serial.println("🚀 Forcing LVGL refresh now...");
    lv_refr_now(NULL);  // **強制刷新 LVGL**
}

// **讀取按鈕並更新畫面**
void check_buttons() {
    if (digitalRead(BTN_BLUE) == LOW) {  // 按鈕按下時 GPIO 為 LOW
        Serial.println("🔵 BLUE Button Pressed!");
        create_random_square(lv_color_make(0, 0, 255));  // 藍色方塊
        delay(200);  // 按鍵防抖
    }
    if (digitalRead(BTN_RED) == LOW) {  // 按鈕按下時 GPIO 為 LOW
        Serial.println("🔴 RED Button Pressed!");
        create_random_square(lv_color_make(255, 0, 0));  // 紅色方塊
        delay(200);  // 按鍵防抖
    }
}

// **LVGL Flush Callback**
void my_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    Serial.println("🔥 ENTERED flush_cb!");

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

    Serial.println("✅ Flush completed!");
    lv_disp_flush_ready(disp);  // **這行一定要執行，告知 LVGL 刷新已完成**
}

// **Setup**
void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Initializing M5Stack CoreS3...");

    // **初始化 M5Stack**
    auto cfg = M5.config();
    cfg.clear_display = true;
    cfg.output_power = true;
    M5.begin(cfg);

    // **初始化 GPIO**
    pinMode(BTN_BLUE, INPUT_PULLUP);
    pinMode(BTN_RED, INPUT_PULLUP);

    // **初始化 LVGL**
    Serial.println("🚀 Initializing LVGL...");
    lv_init();

    // **設定 LVGL 顯示緩衝區**
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 320 * 10);
    Serial.println("✅ LVGL Buffer Initialized");

    // **初始化 LVGL 顯示驅動**
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf;

    // **註冊顯示驅動**
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    Serial.println("✅ Registered LVGL Display Driver");

    // **檢查 `flush_cb` 是否真的被註冊**
    // Serial.print("Checking if flush_cb is set: ");
    // Serial.println(lv_disp_get_flush_cb(lv_disp_get_default()) ? "✅ Exists" : "❌ NULL");

    // **建立 UI 畫面**
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);  // 白色背景

    // **建立棋盤格**
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            grid[x][y] = lv_obj_create(screen);
            lv_obj_set_size(grid[x][y], CELL_SIZE, CELL_SIZE);
            lv_obj_set_style_border_color(grid[x][y], lv_color_black(), LV_PART_MAIN);
            lv_obj_set_style_border_width(grid[x][y], 1, LV_PART_MAIN);
            lv_obj_align(grid[x][y], LV_ALIGN_TOP_LEFT, BOARD_X + x * CELL_SIZE, BOARD_Y + y * CELL_SIZE);
        }
    }
    Serial.println("✅ UI Created!");
}

// **Loop**
void loop() {
    check_buttons();  // 讀取按鈕訊號
    lv_timer_handler();
    delay(50);
}
