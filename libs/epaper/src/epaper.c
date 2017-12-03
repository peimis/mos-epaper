#include "mgos.h"
#include "mgos_config.h"
#include "mgos_spi.h"
#include "epaper.h"

static int _width=0;
static int _height=0;
static int _cs_pin=0;
static int _dc_pin=0;
static int _busy_pin=0;
static int _reset_pin=0;
static bool	_isdirty = true;

static enum epaper_update_type_t _lut = FULL_UPDATE;

static struct mgos_spi *epaper_spi;
static struct mgos_spi_txn epaper_txn;
#if defined(USE_GLOBAL_SPI) && (USE_GLOBAL_SPI == 0)
static struct mgos_config_spi epaper_bus_cfg;
#endif

typedef struct {
	uint8_t data[30];
} lut_data_t;


lut_data_t lut[2] = {
	{
	.data = {
		0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
		0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
		0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00
		}
	},
	{
	.data = {	
		0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}
	}
};


// --------------------------------------------------------------------------------------
//

static int mgos_epd_send_command(const uint8_t cmd_byte);
static int mgos_epd_send_data(const uint8_t data_byte);
static int mgos_epd_send_data_n(const uint8_t * const data, const int len);


static void ep_delay(const int ms)
{
	mgos_msleep(ms);
}



// --------------------------------------------------------------------------------------
//


/**
 *  @brief: A primitive function to Reset the ePaper
 */
int mgos_epd_reset(void)
{
	mgos_gpio_write(_dc_pin, 0);

	mgos_gpio_write(_reset_pin, 0);                //module reset    
	ep_delay(200);
	mgos_gpio_write(_reset_pin, 1);
	ep_delay(200);

	return 0;
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void mgos_epd_wait_idle(void)
{
	int busy, count=0;

	while (0 != (busy = mgos_gpio_read(_busy_pin)))
	{
		ep_delay(50);
		LOG(LL_ERROR, ("Still BUSY .. %d", ++count));
	}
	// LOG(LL_INFO, ("Busy inactive"));
}



void mgos_epd_sleep(void)
{
	mgos_epd_send_command(DEEP_SLEEP_MODE);
	mgos_epd_wait_idle();
}


/**
 *  @brief: Push an image buffer to the frame memory.
 *          this won't update the display.
 *
 */
void mgos_epd_pushFrameBuffer(const uint8_t* framebuffer, const int start_x, const int start_y, const int image_width, const int image_height)
{
	int x_end, y_end;

	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	int adj_image_width = image_width & 0xF8;
	int adj_x = start_x & 0xF8;
	int bufferlen;

	if ( (framebuffer == NULL) || (adj_x < 0) || (image_width < 0) || (start_y < 0) || (image_height < 0) ) {
		return;
	}

	if (adj_x + adj_image_width >= _width) {
		x_end = _width - 1;
	} else {
		x_end = adj_x + adj_image_width - 1;
	}

	if (start_y + image_height >= _height) {
		y_end = _height - 1;
	} else {
		y_end = start_y + image_height - 1;
	}

	mgos_epd_set_memory_area(adj_x, start_y, x_end, y_end);
	mgos_epd_set_memory_pointer(adj_x, start_y);

	mgos_epd_send_command(WRITE_RAM);

	/* send the image data */
	bufferlen = (y_end - start_y + 1) * ((x_end - adj_x + 1) / 8);
	mgos_epd_send_data_n(framebuffer , bufferlen);
}


void mgos_epd_pushFrameBufferRel(const uint8_t* framebuffer, const int start_x, const int start_y, const int image_width, const int image_height)
{
	int x_end, y_end;

	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	int adj_image_width = image_width & 0xF8;
	int adj_x = start_x & 0xF8;
	int bufferlen;

	if ( (framebuffer == NULL) || (adj_x < 0) || (image_width < 0) || (start_y < 0) || (image_height < 0) ) {
		return;
	}

	if (adj_x + adj_image_width >= _width) {
		x_end = _width - 1;
	} else {
		x_end = adj_x + adj_image_width - 1;
	}

	if (start_y + image_height >= _height) {
		y_end = _height - 1;
	} else {
		y_end = start_y + image_height - 1;
	}

	mgos_epd_send_command(WRITE_RAM);

	/* send the image data */
	bufferlen = (y_end - start_y + 1) * ((x_end - adj_x + 1) / 8);
	mgos_epd_send_data_n(framebuffer , bufferlen);
}


/**
 *  @brief: clear the frame memory with the specified color.
 *          this won't update the display.
 */
void mgos_epd_clear_frame_memory(const uint8_t color)
{
	int i;

	mgos_epd_set_memory_area(0, 0, _width - 1, _height - 1);
	mgos_epd_set_memory_pointer(0, 0);

	mgos_epd_send_command(WRITE_RAM);
	// send the color data
	#if 0
	for (i=0; i < ((_width / 8) * _height); i++) {
		mgos_epd_send_data(color);
	}
	#else
	uint8_t *tmp_buffer = (uint8_t *)calloc(1, _height);
	memset(tmp_buffer, color, _height);
	for (i=0; i < (_width / 8); i++) {
		mgos_epd_send_data_n(tmp_buffer, _height);
	}
	free(tmp_buffer);
	#endif
	mgos_epd_wait_idle();
}

/**
 *  @brief: update the display
 *          there are 2 memory areas embedded in the e-paper display
 *          but once this function is called,
 *          the the next action of SetFrameMemory or ClearFrame will 
 *          set the other memory area.
 */
void mgos_epd_display_frame(void)
{
	mgos_epd_send_command(DISPLAY_UPDATE_CONTROL_2);
	mgos_epd_send_data(0xC4);
	mgos_epd_send_command(MASTER_ACTIVATION);
	mgos_epd_send_command(TERMINATE_FRAME_READ_WRITE);

	mgos_epd_wait_idle();
}


/**
 *  @brief: private function to specify the memory area for data R/W
 */
void mgos_epd_set_memory_area(const int x_start, const int y_start, const int x_end, const int y_end)
{
	mgos_epd_send_command(SET_RAM_X_ADDRESS_START_END_POSITION);
	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	mgos_epd_send_data((x_start >> 3) & 0xFF);
	mgos_epd_send_data((x_end >> 3) & 0xFF);
	mgos_epd_send_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
	mgos_epd_send_data(y_start & 0xFF);
	mgos_epd_send_data((y_start >> 8) & 0xFF);
	mgos_epd_send_data(y_end & 0xFF);
	mgos_epd_send_data((y_end >> 8) & 0xFF);
}

/**
 *  @brief: private function to specify the start point for data R/W
 */
void mgos_epd_set_memory_pointer(const int x, const int y)
{
	mgos_epd_send_command(SET_RAM_X_ADDRESS_COUNTER);
	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	mgos_epd_send_data((x >> 3) & 0xFF);
	mgos_epd_send_command(SET_RAM_Y_ADDRESS_COUNTER);
	mgos_epd_send_data(y & 0xFF);
	mgos_epd_send_data((y >> 8) & 0xFF);

	mgos_epd_wait_idle();
}


/**
 *  @brief: set the look-up table register
 */
void mgos_epd_set_lut(enum epaper_update_type_t lut_type)
{
	uint8_t *p;

	_lut = lut_type;

	if (lut_type > PARTIAL_UPDATE) {
		LOG(LL_ERROR, ("Invalid LUT type %d", lut_type));
		return;
	}

	p=(uint8_t *)&lut[lut_type];

	mgos_epd_send_command(WRITE_LUT_REGISTER);

	/* the length of look-up table is 30 bytes */
	mgos_epd_send_data_n(p, sizeof(lut_data_t));
}


/**
 *  @brief: Initialize the display
 */
bool mgos_epd_display_init(enum epaper_update_type_t type)
{
	int paper_height = mgos_sys_config_get_epaper_size_y();

	LOG(LL_INFO, ("Init ePaper display .."));

	_lut = type;
	mgos_epd_reset();

	mgos_epd_send_command(DRIVER_OUTPUT_CONTROL);
	mgos_epd_send_data((paper_height - 1) & 0xFF);
	mgos_epd_send_data(((paper_height - 1) >> 8) & 0xFF);
	mgos_epd_send_data(0x00);                     // GD = 0; SM = 0; TB = 0;
	mgos_epd_send_command(BOOSTER_SOFT_START_CONTROL);
	mgos_epd_send_data(0xD7);
	mgos_epd_send_data(0xD6);
	mgos_epd_send_data(0x9D);
	mgos_epd_send_command(WRITE_VCOM_REGISTER);
	mgos_epd_send_data(0xA8);                     // VCOM 7C
	mgos_epd_send_command(SET_DUMMY_LINE_PERIOD);
	mgos_epd_send_data(0x1A);                     // 4 dummy lines per gate
	mgos_epd_send_command(SET_GATE_TIME);
	mgos_epd_send_data(0x08);                     // 2us per line
	mgos_epd_send_command(DATA_ENTRY_MODE_SETTING);
	mgos_epd_send_data(0x03);                     // X increment; Y increment

	mgos_epd_set_lut( type );

	return 0;
}


/**
 *  @brief: A primitive function to push data to Mongoose-SPI
 */
static int mgos_epd_write_spi(const uint8_t * const data, const int len)
{
	/* Half-duplex, command/response transaction setup */
	/* Transmit 1 byte from tx_data. */
	epaper_txn.fd.len = len;
	epaper_txn.fd.tx_data = data;
	epaper_txn.fd.rx_data = NULL;

	if (!mgos_spi_run_txn(epaper_spi, true /* full_duplex */, &epaper_txn)) {
		LOG(LL_ERROR, ("SPI transaction failed"));
		return -1;
	}

	return 0;
}


/**
 *  @brief: A primitive to send a commend to ePaper
 */
static int mgos_epd_send_command(const uint8_t cmd_byte)
{
	const uint8_t data = cmd_byte;

	mgos_gpio_write( _dc_pin, 0);

	return mgos_epd_write_spi(&data, 1);
}

/**
 *  @brief: A primitive to send a single byte of data to ePaper
 */
static int mgos_epd_send_data(const uint8_t data_byte)
{
	const uint8_t data = data_byte;

	mgos_gpio_write( _dc_pin, 1);

	return mgos_epd_write_spi(&data, 1);
}

/**
 *  @brief: A primitive function to push data to Mongoose-SPI
 */
static int mgos_epd_send_data_n(const uint8_t * const data, const int len)
{
	mgos_gpio_write( _dc_pin, 1);

	epaper_txn.fd.len = len;
	epaper_txn.fd.tx_data = data;
	epaper_txn.fd.rx_data = NULL;

	if (!mgos_spi_run_txn(epaper_spi, true /* full_duplex */, &epaper_txn)) {
		LOG(LL_ERROR, ("SPI transaction failed"));
		return -1;
	}

	return 0;
}

void mgos_epdUpdateNeeded(void)
{
	_isdirty = true;
}

void mgos_epdUpdate(void)
{
	if (_isdirty) {
		mgos_epd_display_frame();
		_isdirty = false;
	}
}

/**
 * Mongoose init
 */
bool mgos_epaper_init(void)
{
	LOG(LL_INFO, ("Set SPI .."));

	/* Global SPI instance is configured by the `spi` config section. */
  /* Alternatively, you can set up the bus manually. */
#if defined(USE_GLOBAL_SPI) && (USE_GLOBAL_SPI == 0)
	struct mgos_config_spi ep_bus_cfg = {
	  .unit_no = 2,
	  .miso_gpio = mgos_sys_config_get_spi_miso_gpio(),
	  .mosi_gpio = mgos_sys_config_get_spi_mosi_gpio(),
	  .sclk_gpio = mgos_sys_config_get_spi_sclk_gpio(),
	  .cs0_gpio = mgos_sys_config_get_epaper_cs_pin(),
	  .debug = true,
	};
#endif
	LOG(LL_INFO, ("OK."));

	_width = mgos_sys_config_get_epaper_size_x();
	_height = mgos_sys_config_get_epaper_size_y();

	_dc_pin = mgos_sys_config_get_epaper_dc_pin();
	mgos_gpio_write(_dc_pin, 1);
	mgos_gpio_set_mode(_dc_pin, MGOS_GPIO_MODE_OUTPUT);

	_cs_pin = mgos_sys_config_get_epaper_cs_pin();
/*
	mgos_gpio_write(_cs_pin, 1);
	mgos_gpio_set_mode(_cs_pin, MGOS_GPIO_MODE_OUTPUT);
*/
	_reset_pin = mgos_sys_config_get_epaper_reset_pin();
	mgos_gpio_write(_cs_pin, 1);
	mgos_gpio_set_mode(_reset_pin, MGOS_GPIO_MODE_OUTPUT);

	_busy_pin = mgos_sys_config_get_epaper_busy_pin();
	mgos_gpio_set_pull(_busy_pin, MGOS_GPIO_PULL_UP);
	mgos_gpio_set_mode(_busy_pin, MGOS_GPIO_MODE_INPUT);

	LOG(LL_INFO, ("cs=%d d/c=%d busy=%d reset=%d", _cs_pin, _dc_pin, _busy_pin, _reset_pin));

#if defined(USE_GLOBAL_SPI) && (USE_GLOBAL_SPI == 0)
	epaper_spi = mgos_spi_create(&ep_bus_cfg);
	epaper_bus_cfg = ep_bus_cfg;
#else
	epaper_spi = mgos_spi_get_global();
#endif

	if (epaper_spi == NULL) {
		LOG(LL_ERROR, ("SPI is not configured, make sure spi.enable is true"));
		return 0;
	}

	struct mgos_spi_txn txn = {
	  .cs = 0,		/* Use CS0 line as configured by cs0_gpio */
	  .mode = 0,
	  .freq = 500000,
	};
	epaper_txn = txn;

	if (0 != mgos_epd_display_init( FULL_UPDATE )) {
		LOG(LL_ERROR, ("Could not initialize ePaper display"));
	}

	return true;
}
