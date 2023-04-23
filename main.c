/**

 */
/* Example code to talk to a Microchip Technology Inc. EEPROM 8K-256K SPI Serial EEPROM.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore SPI) cannot be used at 5v.

   You will need to use a level shifter on the SPI lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board and a generic ee25xx device, other
   boards may vary.

   GPIO 16 (pin 21) MISO/spi0_rx-> SDO/SDO on ee25xx device
   GPIO 17 (pin 22) Chip select -> CSB/!CS on ee25xx device
   GPIO 18 (pin 24) SCK/spi0_sclk -> SCL/SCK on ee25xx device
   GPIO 19 (pin 25) MOSI/spi0_tx -> SDA/SDI on ee25xx device
   3.3v (pin 36) -> VCC on ee25xx device
   GND (pin 38)  -> GND on ee25xx device


*/
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"



#define READ  0b00000011 //Read data from memory array beginning at selected address
#define WRITE 0b00000010 //Write data to memory array beginning at selected address
#define WRDI  0b00000100 //Reset the write enable latch (disable write operations)
#define WREN  0b00000110 //Set the write enable latch (enable write operations)
#define RDSR  0b00000101 //Read STATUS register
#define WRSR  0b00000001 //Write STATUS register

#define PAGE_SIZE 16 // Number of bytes per page. Depends on EEPROM model.

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

void pico_spi_init(){
    // This will use SPI0 at 0.5MHz.
    spi_init(spi_default, 500 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));
}
static void write_registers(uint8_t data) {

    cs_select();
    spi_write_blocking(spi_default, &data, 1);
    cs_deselect();
    sleep_ms(10);
}
static void EE25XX_Read_Status_Register(uint8_t * read_data) {

    uint8_t reg = RDSR;
    cs_select();
    spi_write_blocking(spi_default, &reg, 1);
    spi_read_blocking(spi_default, 0, read_data, 1);
    cs_deselect();
    sleep_ms(10);
}
void Print_Data_Debug(uint8_t * data, uint8_t length){

    for(uint8_t i = 0 ; i < length; i++){
        printf("0x%x ",data[i]);
    }
    printf("\n");
}
void Print_Status_Register(){
    uint8_t data = 0;

    printf("Reading Status register\n");
    EE25XX_Read_Status_Register(&data);
    printf("Read data data = 0x%x\n",data);
}
void EE25XX_Write_Enable(){
    write_registers((uint8_t)WREN);
}

void EE25XX_Write_Disable(){
    write_registers((uint8_t)WRDI);
}

void EE25XX_Write_Page(uint8_t * write_data, uint16_t address){

    uint8_t COMMAND = WRITE;
    uint8_t addr_h = (uint16_t) address >> 8;
    uint8_t addr_l = address;


    cs_select();
    spi_write_blocking(spi_default, &COMMAND, 1);
    spi_write_blocking(spi_default, &addr_l, 1);
    spi_write_blocking(spi_default, &addr_h, 1);

    spi_write_blocking(spi_default, write_data, PAGE_SIZE);
    cs_deselect();
    sleep_ms(10);
}
void EE25XX_Write_Byte(uint8_t * write_byte, uint16_t address){

    uint8_t COMMAND = WRITE;
    uint8_t addr_h = (uint16_t) address >> 8;
    uint8_t addr_l = address;


    cs_select();
    spi_write_blocking(spi_default, &COMMAND, 1);
    spi_write_blocking(spi_default, &addr_l, 1);
    spi_write_blocking(spi_default, &addr_h, 1);

    spi_write_blocking(spi_default, write_byte, 1);
    cs_deselect();
    sleep_ms(10);
}

void EE25XX_Read_Page(uint8_t * read_data, uint16_t address){
    
    uint8_t COMMAND = READ;
    uint8_t addr_h = (uint16_t) address >> 8;
    uint8_t addr_l = address;

    cs_select();
    spi_write_blocking(spi_default, &COMMAND, 1);
    spi_write_blocking(spi_default, &addr_l, 1);
    spi_write_blocking(spi_default, &addr_h, 1);

    spi_read_blocking(spi_default, 0, read_data, PAGE_SIZE);
    cs_deselect();
    sleep_ms(10);
}

int main() {
    
    stdio_init_all();


    pico_spi_init();



    printf("Hello, 25xx eeprom! Writing and Reading a page from the EEPROM via SPI...\n");
    uint8_t data_tx[16] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC };
    uint8_t data_rx[16] = { 0 };
    uint16_t address = 0x000;

    while (1) {
    EE25XX_Write_Enable();
    Print_Status_Register();

    EE25XX_Write_Page(data_tx, address);
    EE25XX_Read_Page(data_rx, address);

    Print_Data_Debug(data_rx, PAGE_SIZE);

    EE25XX_Write_Disable();
    Print_Status_Register();
    sleep_ms(100);

    }

    return 0;
}