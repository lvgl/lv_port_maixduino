#include <lvgl.h>
#include <Ticker.h>
#include <Sipeed_ST7789.h>
#include <touchscreen.h>

#define LVGL_TICK_PERIOD 20

SPIClass spi_(SPI0);// MUST be SPI0 for Maix series on board LCD
Ticker tick; /* timer for interrupt handler */
Sipeed_ST7789 lcd(320, 240, spi_);
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];
TouchScreen touchscreen;
int ledstate = 1;
#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

  Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  delay(100);
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int32_t w = area->x2-area->x1+1;
  int32_t h = area->y2-area->y1+1;
  int32_t x,y;
  int32_t i=0;
  uint16_t* data = (uint16_t*)malloc( w*h*2 );
  uint16_t* pixels = data;

  for(y=area->y1; y<=area->y2; ++y)
  {
    for(x=area->x1; x<=area->x2; ++x)
    {
      pixels[i++]= (color_p->ch.red<<3) | (color_p->ch.blue<<8) | (color_p->ch.green>>3&0x07 | color_p->ch.green<<13);
      // or LV_COLOR_16_SWAP = 1
       ++color_p;
    }
  }
  lcd.drawImage((uint16_t)area->x1, (uint16_t)area->y1, (uint16_t)w, (uint16_t)h, data);
  free(data);
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/* Interrupt driven periodic handler */
static void lv_tick_handler(void)
{

  lv_tick_inc(LVGL_TICK_PERIOD);
}

/* Reading input device  */
bool read_touchscreen(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
  int status, x, y;
  touchscreen.read();
  status = touchscreen.getStatus();
  x = touchscreen.getX();
  y = touchscreen.getY();
  switch(status)
  {
    case TOUCHSCREEN_STATUS_RELEASE:
        data->state =  LV_INDEV_STATE_REL;
      break;
    case TOUCHSCREEN_STATUS_PRESS:
    case TOUCHSCREEN_STATUS_MOVE:
      data->state = LV_INDEV_STATE_PR;
      break;
    default:
      return false;
  }
  data->point.x = x;
  data->point.y = y;
  return false;
}

static void event_handler(lv_obj_t * btn, lv_event_t event)
{
  if(event == LV_EVENT_CLICKED) {
    Serial.printf("Clicked\n");
  }
  else if(event == LV_EVENT_VALUE_CHANGED) {
    Serial.printf("Toggled\n");
  }
}

void setup() {

  Serial.begin(115200); /* prepare for possible serial debug */
  lcd.begin(15000000, COLOR_WHITE);
  touchscreen.begin();
  lv_init();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN,ledstate);//power off led
#if USE_LV_LOG != 0
  lv_log_register_print(my_print); /* register print function for debugging */
#endif

  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);


  /*Initialize the touch pad*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = read_touchscreen;
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);

  /*Initialize the graphics library's tick*/
  tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);

  /* Create simple label */
  lv_obj_t * label;

  lv_obj_t * btn1 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn1, event_handler);
  lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, -40);

  label = lv_label_create(btn1, NULL);
  lv_label_set_text(label, "Button");

  lv_obj_t * btn2 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn2, event_handler);
  lv_obj_align(btn2, NULL, LV_ALIGN_CENTER, 0, 40);
  lv_btn_set_toggle(btn2, true);
  lv_btn_toggle(btn2);
  lv_btn_set_fit2(btn2, LV_FIT_NONE, LV_FIT_TIGHT);

  label = lv_label_create(btn2, NULL);
  lv_label_set_text(label, "Toggled");
}


void loop() {

  lv_task_handler(); /* let the GUI do its work */
  delay(5);
}
