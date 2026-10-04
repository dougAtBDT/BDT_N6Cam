/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "isp_api.h"
#include "imx335_E27_isp_param_conf.h"
//#define USE_DISCO_KIT
//#define USE_DCACHE
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//#define SEND_3GRAYS

//#define RESOLUTION_LCD	//640*480*3=384,400.
//#define RESOLUTION_1200_800	//1200x800x3=2,880,000.  IMX355 MAX = 2582 X 1944. Should be able get more than 1200x800 if can stream to external RAM.
#define RESOLUTION_2400_1600	//2400x1600x3=11,520,000.  IMX355 MAX = 2582 X 1944 x 3 = 15,058,224. Should be able get more than 1200x800 if can stream to external RAM.

#define ISP_OK 0	//I don't know why the compiler can't find this enum in isp_core.h
#define EE_I2C_DELAY (20)
#define TICKCTR 5	//used in myDelay() along with SAM_CLK_DELAY_READ/WRITE. Had to modify after SDIO clock modifications.
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DCMIPP_HandleTypeDef hdcmipp;
ISP_HandleTypeDef  hcamera_isp;
PCD_HandleTypeDef hpcd_USB_OTG_HS1;
XSPI_HandleTypeDef hxspi1;
XSPI_HandleTypeDef hxspi2;

UART_HandleTypeDef huart10;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

static __IO uint32_t NbMainFrames = 0;
static IMX335_Object_t   IMX335Obj;
static int32_t isp_gain;
static int32_t isp_exposure;

static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat);
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo);
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain);
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain);
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure);
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure);

/* Preview frame buffer */
#ifdef RESOLUTION_LCD	//800*480*3=384,400.
#define MAX_PREVIEW_BUFFER_WIDTH    640	//640
#define MAX_PREVIEW_BUFFER_HEIGHT   480	//480
/* Allocate the Main_DestBuffer (RGB888) in SRAM dedicated region */
//__attribute__ ((section(".buffRam")))
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t Main_DestBuffer[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT * 3];	//921,600 Bytes of 4.2MBytes on N6

__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_A[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];

__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_B[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];
#endif
#ifdef RESOLUTION_1200_800	//	640*480*3=921,600
#define MAX_PREVIEW_BUFFER_WIDTH    640	//640
#define MAX_PREVIEW_BUFFER_HEIGHT   480
/* Allocate the Main_DestBuffer (RGB888) in SRAM dedicated region */
//__attribute__ ((section(".buffRam")))
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned (32)))
//uint8_t Main_DestBuffer[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT * 3];	//921,600 Bytes of 4.2MBytes on N6
//uint8_t Main_DestBuffer[640 * 480 * 3];	//1200x800x3=2,880,000.  Need to deal with image size at end of ISP_IQParamTypeDef.  IMX355 MAX = 2582 X 1944. Should be able get more than 1200x800 if can DMA to external RAM.

#ifdef SEND_3GRAYS
__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_A[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];

__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_B[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];
#endif //three grayscales
#endif
#ifdef RESOLUTION_2400_1600	//	2400*1600*3 = 11,520,000
/* Image width: at 2400 (7200 bytes per line) PSRAM must empty each line before the next one;
 * at 75MHz without the HSLV fuse it fell a few hundred pixels short. 1920 (5760 bytes per
 * line) gives PSRAM time to catch up if needed. Must keep IMAGE_WIDTH * 3 a
 * multiple of 16 (DCMIPP pitch) and IMAGE_WIDTH * IMAGE_HEIGHT * 3 a multiple of 9600
 * (8 colour bars x 1200-byte packets); 1920, 2048 and 2400 all qualify.
 * The receiver must expect the same width. */
#define IMAGE_WIDTH    2400	//1920 if PSRAM can't keep up
#define IMAGE_HEIGHT   1600
#define IMAGE_PACKETS  (IMAGE_WIDTH * IMAGE_HEIGHT * 3 / 1200)	//1200-byte UART packets per image
_Static_assert((IMAGE_WIDTH * 3) % 16 == 0, "DCMIPP pitch must be a multiple of 16 bytes");
_Static_assert((IMAGE_WIDTH * IMAGE_HEIGHT * 3) % 9600 == 0, "image must split into 8 bars of 1200-byte packets");
_Static_assert(IMAGE_WIDTH <= 2592 && IMAGE_HEIGHT <= 1944, "crop larger than the IMX335 sensor");
/* Sensor lines are stretched by this factor (HMAX register) so PSRAM can absorb each 7200-byte
 * line: 7.4us -> 74us per line, ~97 MB/s, ~3 fps. The exposure helpers divide by it so the
 * IMX335 driver, which assumes 7.4us lines, still sets the right number of lines. */
#define CAM_LINE_SCALE 10
/* Allocate the Main_DestBuffer (RGB888) in SRAM dedicated region */
//__attribute__ ((section(".buffRam")))
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t Main_DestBuffer[IMAGE_WIDTH * IMAGE_HEIGHT * 3];	//2400*1600*3 = 11,520,000.  Need to deal with image size at end of ISP_IQParamTypeDef.  IMX355 MAX = 2582 X 1944.
__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_A[IMAGE_WIDTH * IMAGE_HEIGHT];

__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_B[IMAGE_WIDTH * IMAGE_HEIGHT];

#ifdef SEND_3GRAYS
__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_A[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];

__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t grayscale_B[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];
#endif //three grayscales
#endif

//__attribute__ ((section(".buffRam")))
//__attribute__ ((aligned (32)))
//uint8_t grayscale_C[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT];

#ifndef CAM_LINE_SCALE
#define CAM_LINE_SCALE 1	//normal sensor line timing
#endif
#define IMX335_REG_HMAX 0x3034	//line length, 16 bit (not in the ST driver's register list)
/* Uncomment to make the IMX335 output a built-in test pattern instead of the scene, to check
 * the whole sensor -> DCMIPP/ISP -> PSRAM -> UART path. 10 = horizontal colour bars,
 * 11 = vertical colour bars (see IMX335_SetTestPattern for the others). */
#define CAM_TEST_PATTERN 11

#define USE_HAL_DCMIPP_REGISTER_CALLBACKS 1

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_DCMIPP_Init(void);
static void MX_GPIO_Init(void);
//static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART10_UART_Init(void);
static void MX_USB1_OTG_HS_PCD_Init(void);
static void MX_XSPI2_Init(void);	//Macronix Flash
static void MX_XSPI1_Init(void);	//AP Memeory PSRAM
/* USER CODE BEGIN PFP */
//ISP_HandleTypeDef  hcamera_isp;	//from Snapshot
XSPI_MemoryMappedTypeDef sMemMappedCfg;

void ledBlink (uint8_t);
void LOOP (void);
uint8_t stm_xspi_psram_test(uint8_t);

uint32_t APS256_WriteReg(XSPI_HandleTypeDef *Ctx, uint32_t Address, uint8_t *Value);
uint32_t APS256_ReadReg(XSPI_HandleTypeDef *Ctx, uint32_t Address, uint8_t *Value, uint32_t LatencyCode);
static void Configure_APMemory(void);
void psramInitFrmMX(void);
uint8_t psramTest(uint8_t, uint8_t);
uint32_t psramAddrTest(uint32_t *, uint32_t);
uint8_t psramTest32Bit(uint8_t);
void printDumpBuff (void);
void clearDumpBuff (void);
void sendPic(void);
void sendColorBars(void);
void send3Grays(void);
void sendColorBars(void);

uint16_t getALSfromTinyAPDS(uint8_t);
void sensorSCL_OUT(void);
void sensorSCL_OUTlow(void);
void sensorSDA_OUTlow(void);
void sensorSDA_OUT(void);
void sensorSCL_INpull(void);
void sensorSDA_INpull(void);
void write_sda_L (uint8_t);
uint8_t i2c_read_byte_L(uint8_t*, uint8_t, uint8_t);
uint8_t read_bytes_L(uint8_t*, uint8_t, uint8_t);
uint8_t i2c_write_byte_L(uint8_t, uint8_t);
uint8_t write_data_L(uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t twi_start_cond_L(void);
uint8_t twi_stop_cond_L(void);
uint8_t send_slave_Address_L(uint8_t, uint8_t);
void sensorSDA_OUT(void);
void myDelay(uint32_t);

void UARTint(uint32_t);
void UARTshort(uint16_t, uint8_t);
void UARTreturn(void);
void UARTspace(void);
uint8_t hex1ToAscii(uint8_t);
uint32_t hex2ToAscii(uint32_t);
void UARTint(uint32_t);
void UARTshort(uint16_t, uint8_t);
void UARTreturn(void);
void UARTspace(void);
uint8_t hex1ToAscii(uint8_t);
uint32_t hex2ToAscii(uint32_t);
unsigned short calcCRC(unsigned char*, unsigned int);

void printDestBuff (void);
void clearDestBuff (void);
void createGrayscale (uint8_t);
void ispBackground(void);
void captureCheckPrepare(void);
void captureCheckReport(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	  ISP_AppliHelpersTypeDef appliHelpers = {0};
	  DCMIPP_DecimationConfTypeDef pDecConfig = {0};
	  uint32_t pitch_value ; /* Number of bytes per line */
	  uint8_t delayForAuto;
	  uint16_t als;

	  XSPI_RegularCmdTypeDef sCommand = {0};
	  uint32_t camera_instance = 0;
	  ISP_StatusTypeDef ret;

	  /* Power on ICACHE */
	  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_ICACTIVE_Msk;

	  /* Set back system and CPU clock source to HSI */
	  __HAL_RCC_CPUCLK_CONFIG(RCC_CPUCLKSOURCE_HSI);
	  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

	  SCB_EnableICache();

	#if defined(USE_DCACHE)
	  /* Power on DCACHE */
	  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_DCACTIVE_Msk;
//	  SCB_EnableDCache();
	#endif

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */
  uint32_t iters;
	uint16_t crcrc;
	uint8_t crcTestArray[20];
	uint8_t res;
	uint8_t errr;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();


  /* USER CODE BEGIN SysInit */
//  uint32_t pclk1Freq = LL_RCC_CALC_PCLK1_FREQ(LL_RCC_CALC_HCLK_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHBPrescaler()),
//                                  LL_RCC_GetAPB1Prescaler());
//  uint32_t pclk5Freq = LL_RCC_CALC_PCLK5_FREQ(LL_RCC_CALC_HCLK_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHBPrescaler()),
//                                  LL_RCC_GetAPB1Prescaler());

#ifdef RESOLUTION_1200_800
  /* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
 // this setup is included in code
 void nsDelay(uint32_t ns) {
  volatile uint32_t count = ns / 1.25; // 1.25 for 800mhz MCU clock
  while (count--) {
  __NOP(); // 1 cycle (~1ns on 100MHz+ CPU)
  }
 }
  /* Unlock RAMCFG write protection */
	RAMCFG_SRAM3_AXI_NS->ERKEYR = 0xCA;
	RAMCFG_SRAM3_AXI_NS->ERKEYR = 0x53;

	/* Enable RAMCFG clock */
	__HAL_RCC_RAMCFG_CLK_ENABLE();

	/* Enable individual AXI SRAM memory clocks */
	__HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
	__HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
	__HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
	__HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();

	nsDelay(50); // Provide adequate stabilization delay

	/* Disable the Shutdown mode (SRAMSD) to power up the RAMs */
	/* Note: Use _S instead of _NS if your application is running in Secure Mode */
	RAMCFG_SRAM3_AXI_NS->CR &= ~RAMCFG_CR_SRAMSD;
	RAMCFG_SRAM4_AXI_NS->CR &= ~RAMCFG_CR_SRAMSD;
	RAMCFG_SRAM5_AXI_NS->CR &= ~RAMCFG_CR_SRAMSD;
	RAMCFG_SRAM6_AXI_NS->CR &= ~RAMCFG_CR_SRAMSD;

	/* Insert barriers to ensure configuration takes effect before access */
	__DSB();
	__ISB();
#endif
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_USART10_UART_Init();
  MX_USART1_UART_Init();	//Diags
  MX_XSPI2_Init();
  MX_XSPI1_Init();
//  ledBlink(3);


  MX_GPIO_Init();

  HAL_UART_Transmit(&huart1, (unsigned char*)("\r\nN6 CAM BOOT NORM 03\r\n"), 23, 100);

  /* OTP word 124 holds the HSLV (1.8V high-speed I/O) option bits, including HSLV_VDDIO2,
   * which the PSRAM pins need. Printed raw so the fuse state can be compared before/after
   * programming it with STM32CubeProgrammer. */
  __HAL_RCC_BSEC_CLK_ENABLE();
  HAL_UART_Transmit(&huart1, (unsigned char*)("OTP124 "), 7, 100);
  if (BSEC->SFSRx[124 / 32] & (1UL << (124 % 32))) {
	  UARTint(BSEC->FVRw[124]);
  } else {
	  HAL_UART_Transmit(&huart1, (unsigned char*)("NOT SHADOWED"), 12, 100);
  }
  /* Sticky locks last until the next reset: PROG = fuse programming blocked */
  if (BSEC->SPLOCKx[124 / 32] & (1UL << (124 % 32))) {
	  HAL_UART_Transmit(&huart1, (unsigned char*)(" PROG LOCKED"), 12, 100);
  }
  if (BSEC->SWLOCKx[124 / 32] & (1UL << (124 % 32))) {
	  HAL_UART_Transmit(&huart1, (unsigned char*)(" WRITE LOCKED"), 13, 100);
  }
  UARTreturn();

  psramInitFrmMX();

  if(psramTest(1,0)==0){
	  Error_Handler();	//1=Exhaustive, 0=first time through this test
  }
  if(psramTest(1,1)==0){
	  Error_Handler();	//1=Exhaustive, 0=first time through this test
  }
  if(psramTest(1,1)==0){
	  Error_Handler();	//1=Exhaustive, 0=first time through this test
  }
  psramAddrTest((uint32_t *) Main_DestBuffer, sizeof(Main_DestBuffer));	//needs memory-mapped mode (psramTest(x,0) above)
/*  for(iters=0;iters<30;iters++){
	  if(psramTest(0,1)==0){
		  Error_Handler();	//1=Exhaustive
	  }
  }*/
//  stm_xspi_psram_test(1);

//  clearDestBuff();
//  clearDumpBuff();
//  printDestBuff();


  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  MX_DCMIPP_Init();
  /* Initialize the IMX335 Sensor ----------------------------- */
  IMX335_Probe(IMX335_R2592_1944, IMX335_RAW_RGGB10);
  HAL_UART_Transmit(&huart1, (unsigned char*)("CAM INIT GOOD\r\n"), 15, 100);

//  HAL_GPIO_WritePin(CAM_PWRC_GPIO_Port, CAM_PWRC_Pin|TX_EN_Pin, GPIO_PIN_SET);
  HAL_UART_Transmit(&huart1, (unsigned char*)("ALS: "), 5, 100);
//  HAL_Delay(10);	//
  als = getALSfromTinyAPDS(0xA4);
  UARTshort(als,4);
  UARTspace();
  HAL_Delay(50);
  als = getALSfromTinyAPDS(0xA4);   //from Python March 2025: need a throw-away read with 40+ms between the two after having read temperature/humidity. Have no idea why.
  UARTshort(als,4);
  UARTreturn();

  /* The ALS bit-bang read reconfigures PC1/PH9 (shared with the camera I2C1) as plain GPIO.
   * Re-init I2C1 to restore the AF4 open-drain pins and reset the peripheral, otherwise
   * every IMX335 register write after this point fails (ISP_ERR_ALGO in AEC init). */
  BSP_I2C1_DeInit();
  if (BSP_I2C1_Init() != BSP_ERROR_NONE)
  {
    Error_Handler();
  }


  /* Fill init struct with Camera driver helpers */
  appliHelpers.GetSensorInfo = GetSensorInfoHelper;			//from Snapshot
  appliHelpers.SetSensorGain = SetSensorGainHelper;			//from Snapshot
  appliHelpers.GetSensorGain = GetSensorGainHelper;			//from Snapshot
  appliHelpers.SetSensorExposure = SetSensorExposureHelper;			//from Snapshot
  appliHelpers.GetSensorExposure = GetSensorExposureHelper;			//from Snapshot

  /* Initialize the Image Signal Processing middleware */
#ifdef RESOLUTION_2400_1600
  res = ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, &ISP_IQParamCacheInit_IMX335_2400_1600);  // options for param configs in core/inc/imx335_E27_isp_param_conf.h
#else
//  if((als<4)||(als==0xFF)){	//If dark then use rodentCam settings with no auto gain or exposure
//	  res = ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, &ISP_IQParamCacheInit_IMX335_BDTrodent);  // options for param configs in core/inc/imx335_E27_isp_param_conf.h
//  }else{	//Use auto settings
	  res = ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, &ISP_IQParamCacheInit_IMX335);            // options for param configs in core/inc/imx335_E27_isp_param_conf.h
//  }
#endif

  if(res != ISP_OK){
		  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP INIT FAIL\r\n"), 15, 100);
		  UARTshort(res,4);
		  UARTreturn();
		  Error_Handler();
	  }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP INIT GOOD\r\n"), 15, 100);
	  }

  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0 , (uint8_t *) Main_DestBuffer, DCMIPP_MODE_CONTINUOUS) != HAL_OK)	//DCMIPP_MODE_SNAPSHOT  //DCMIPP_MODE_CONTINUOUS
  {
	  HAL_UART_Transmit(&huart1, (unsigned char*)("PIPE START FAIL\r\n"), 17, 100);
	  Error_Handler();
  }else{
	  HAL_UART_Transmit(&huart1, (unsigned char*)("PIPE START GOOD\r\n"), 17, 100);
  }

  /* Start the Image Signal Processing */

  if ((errr = ISP_Start(&hcamera_isp)) != ISP_OK)
  {
	  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP START FAIL\r\n"), 16, 100);
	  UARTshort(errr,4);
	  UARTreturn();
	  Error_Handler();
  }else{
	  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP START GOOD\r\n"), 16, 100);
  }
/*********************************************************************************************************************/


/*********************************************************************************************************************/
 /* give the ISP 60 frames to set color balance */
  if((als<4)||(als==0xFF)){	//If dark then use rodentCam settings with no auto gain or exposure
	  HAL_GPIO_WritePin(CAM_FLASH_GPIO_Port, CAM_FLASH_Pin, GPIO_PIN_SET);	//Flash Vreg ON
	  delayForAuto=2;
  }else{
	  HAL_GPIO_WritePin(CAM_FLASH_GPIO_Port, CAM_FLASH_Pin, GPIO_PIN_RESET);	//Flash Vreg OFF
      delayForAuto=10;
  }
  iters=0;
  NbMainFrames=0;
  while(NbMainFrames < delayForAuto)	//  30/SEC. WAS 60 BEFORE MARCH 8. 5=BLACK. 15=TOO WHITE. 25=A LITTLE DARK. // options for param configs in core/inc/imx335_E27_isp_param_conf.h
  {
	  iters++;
	  ispBackground();	//auto exposure and auto white balance on each new frame's statistics
  }
  /* stop the acquisition */
  HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0);

  HAL_GPIO_WritePin(CAM_FLASH_GPIO_Port, CAM_FLASH_Pin, GPIO_PIN_RESET);	//IR CAMERA FLASH. VReg.
  HAL_UART_Transmit(&huart1, (unsigned char*)("TOOK TOOK TOOK TOOK PIC\r\n"), 25, 100);

#ifdef SEND_3GRAYS
  createGrayscale(0);
  takePic();
  createGrayscale(1);
  takePic();
  HAL_UART_Transmit(&huart1, (unsigned char*)("END TAKE PIC\r\n"), 10, 100);
  mergeGrayscales();
#else
  takePic();
#endif

  HAL_UART_Transmit(&huart1, (unsigned char*)("END PICS\r\n"), 10, 100);

//  HAL_UART_Transmit(&huart1, (unsigned char*)("\r\nCAM03 NUM FRAMES "), 19, 100);
//  UARTshort(NbMainFrames,2);
//  HAL_UART_Transmit(&huart1, (unsigned char*)("  ITERS "), 8, 100);
//  UARTshort(iters>>16,4);
//  UARTshort(iters&0x0000FFFF,4);
//  UARTreturn();

//  printDestBuff();

  NbMainFrames = 0;

  iters=0;
  while (1)
  {
	  iters++;
	  if((HAL_GPIO_ReadPin(CTS_FROM_SYSCNTR_GPIO_Port, CTS_FROM_SYSCNTR_Pin))==0){
		HAL_UART_Transmit(&huart1, (unsigned char*)("CTS\r\n"), 5, 100);
		HAL_GPIO_WritePin(TX_EN_GPIO_Port, TX_EN_Pin, GPIO_PIN_SET);
		HAL_UART_Transmit(&huart1, (unsigned char*)("SEND PIC\r\n"), 10, 100);
		while((HAL_GPIO_ReadPin(CTS_FROM_SYSCNTR_GPIO_Port, CTS_FROM_SYSCNTR_Pin))==0){};//Wait for SysCntlr to be ready to receive
		HAL_Delay(50);//100 okay. 50 okay. 10 was bad. 20 was okay.
//		sendColorBars();
//		storeColorBars();//IF STORING AND SENDING COLORBARS, NEED TO ADD A DELAY IMMEDIATELY BEFORE RECEIVETOIDLE_DMA on SysCntler: 	HAL_Delay(1200);//NEEDED WHEN STORING AND SENDING COLORBARS.  1000 WAS NOT ENOUGH. 1400 ALSO WORKED.

		#ifdef SEND_3GRAYS
		send3Grays();
#else
		sendPic();
#endif
		break;
	  }
	  if((iters%500000)==0){
		HAL_UART_Transmit(&huart1, (unsigned char*)("."), 1, 100);
	  }
  }

  LOOP();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
/* USER CODE BEGIN CLK 1 */
/* USER CODE END CLK 1 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

	  	  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
	  	  {
	  	    Error_Handler();
	  	  }
		  /** Enable HSI
		  */
		  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
		  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
		  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
		  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
		  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
		  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
		  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
		  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
		  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		  {
		    Error_Handler();
		  }

		  /* Wait HSE stabilization time before its selection as PLL source. */
		  HAL_Delay(HSE_STARTUP_TIMEOUT);

		  /** Get current CPU/System buses clocks configuration and
		 if necessary switch to intermediate HSI clock to ensure target clock can be set
		  */
		  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
		  if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
		     (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
		  {
		    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
		    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
		    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
		    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
		    {
		      /* Initialization Error */
		      Error_Handler();
		    }
		  }

		  /** Initializes the RCC Oscillators according to the specified parameters
		  * in the RCC_OscInitTypeDef structure.
		  */
		  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
		  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
		  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
		  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
		  RCC_OscInitStruct.PLL1.PLLM = 4;
		  RCC_OscInitStruct.PLL1.PLLN = 75;
		  RCC_OscInitStruct.PLL1.PLLFractional = 0;
		  RCC_OscInitStruct.PLL1.PLLP1 = 1;
		  RCC_OscInitStruct.PLL1.PLLP2 = 1;
		  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
		  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
		  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
		  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		  {
		    Error_Handler();
		  }

		  /** Initializes the CPU, AHB and APB buses clocks
		  */
		  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
		                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
		                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
		                              |RCC_CLOCKTYPE_PCLK4;
		  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
		  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
		  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
		  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
		  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
		  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
		  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
		  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
		  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
		  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
		  RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;	//Bus clock 300MHz. Was raised to 8 for PSRAM, but that starved DCMIPP (pipe1 overrun). PSRAM now has its own clock (IC3, see HAL_XSPI_MspInit).
		  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
		  RCC_ClkInitStruct.IC6Selection.ClockDivider = 3;		//WAS 3  CAM04 USES 4.
		  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
		  RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;

		  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
		  {
		    Error_Handler();
		  }

}

/**
  * @brief DCMIPP Initialization Function
  * @param None
  * @retval None
  */
//static void MX_DCMIPP_Init(void)
//{

  /* USER CODE BEGIN DCMIPP_Init 0 */

  /* USER CODE END DCMIPP_Init 0 */

  /* USER CODE BEGIN DCMIPP_Init 1 */

  /* USER CODE END DCMIPP_Init 1 */
  /* USER CODE BEGIN DCMIPP_Init 2 */

  /* USER CODE END DCMIPP_Init 2 */

//}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
//static void MX_I2C1_Init(void)
//{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
/*  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }*/

  /** Configure Analogue filter
  */
/*  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }*/

  /** Configure Digital filter
  */
/*  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }*/
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

//}
/**
  * @brief DCMIPP Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMIPP_Init(void)
{
  /* USER CODE BEGIN DCMIPP_Init 0 */
  /* USER CODE END DCMIPP_Init 0 */
  DCMIPP_PipeConfTypeDef pPipeConf = {0};
  DCMIPP_CSI_PIPE_ConfTypeDef pCSIPipeConf = {0};
  DCMIPP_CSI_ConfTypeDef csiconf = {0};

  /* Set DCMIPP instance */
  hdcmipp.Instance = DCMIPP;
  if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure the CSI */
  csiconf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csiconf.NumberOfLanes   = DCMIPP_CSI_TWO_DATA_LANES;
  csiconf.PHYBitrate      = DCMIPP_CSI_PHY_BT_1600;
  if(HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK)
  {
    Error_Handler();
  }
  /* Configure the Virtual Channel 0 */
  /* Set Virtual Channel config */
  if(HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure the serial Pipe */
  pCSIPipeConf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  pCSIPipeConf.DataTypeIDA  = DCMIPP_DT_RAW10;
  pCSIPipeConf.DataTypeIDB  = DCMIPP_DT_RAW10; /* Don't Care */


  if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pCSIPipeConf) != HAL_OK)
  {
    Error_Handler();
  }

  pPipeConf.FrameRate  = DCMIPP_FRAME_RATE_ALL;
  pPipeConf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;

  /* Set Pitch for Main and Ancillary Pipes */
#ifdef RESOLUTION_2400_1600
  pPipeConf.PixelPipePitch  = IMAGE_WIDTH * 3;	//bytes per RGB888 line
#else
  pPipeConf.PixelPipePitch  = 1920;	//1600 ; /* Number of bytes per line. for RGB888 = 640*3*/
#endif

  /* Configure Pipe */
  if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pPipeConf) != HAL_OK)
  {
    Error_Handler();
  }

  /* RGB888 is stored B,G,R in memory by default; the receiver expects R,G,B */
  if (HAL_DCMIPP_PIPE_EnableRedBlueSwap(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    Error_Handler();
  }

#ifdef RESOLUTION_2400_1600
  /* Full resolution: crop the centre 2400x1600 of the 2592x1944 sensor, no downsize */
  DCMIPP_CropConfTypeDef cropConf = {0};
  cropConf.HStart   = (IMX335_WIDTH - IMAGE_WIDTH) / 2;	//centred
  cropConf.VStart   = (IMX335_HEIGHT - IMAGE_HEIGHT) / 2;	//172
  cropConf.HSize    = IMAGE_WIDTH;
  cropConf.VSize    = IMAGE_HEIGHT;
  cropConf.PipeArea = DCMIPP_POSITIVE_AREA;
  if (HAL_DCMIPP_PIPE_SetCropConfig(&hdcmipp, DCMIPP_PIPE1, &cropConf) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DCMIPP_PIPE_EnableCrop(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    Error_Handler();
  }
#else
  DCMIPP_DownsizeTypeDef DonwsizeConf ={0};

  /* Configure the downsize */
  DonwsizeConf.HRatio      = 33161;	//25656;  for Disco LCD
  DonwsizeConf.VRatio      = 33161;
  DonwsizeConf.HSize       = 640;	//800
  DonwsizeConf.VSize       = 480;
  DonwsizeConf.HDivFactor  = 253;	//316	for Disco LCD
  DonwsizeConf.VDivFactor  = 253;

  if(HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &DonwsizeConf) != HAL_OK)
  {
    Error_Handler();
  }
  if(HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    Error_Handler();
  }
#endif
  /* USER CODE BEGIN DCMIPP_Init 2 */
  /* The DCMIPP AXI master (IP-Plug) defaults to 8-byte bursts, and each burst becomes its own
   * PSRAM transaction (command + address + write latency). Use 128-byte bursts for client 2
   * (pipe1). FIFO range as in ST's examples: 0..559 (x 8 bytes). Do not go up to 0x3FF:
   * the register accepts it, but the FIFO memory is smaller, and data that passes through the
   * missing part comes out as 0xFF (3KB of every 8KB of image was 0xFF with 0x3FF). */
  DCMIPP_IPPlugConfTypeDef ipplugConf = {0};
  ipplugConf.Client = DCMIPP_CLIENT2;
  ipplugConf.MemoryPageSize = DCMIPP_MEMORY_PAGE_SIZE_256BYTES;
  ipplugConf.Traffic = DCMIPP_TRAFFIC_BURST_SIZE_128BYTES;
  ipplugConf.MaxOutstandingTransactions = DCMIPP_OUTSTANDING_TRANSACTION_NONE;
  ipplugConf.DPREGStart = 0;
  ipplugConf.DPREGEnd = 559;
  ipplugConf.WLRURatio = 15;
  if (HAL_DCMIPP_SetIPPlugConfig(&hdcmipp, &ipplugConf) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END DCMIPP_Init 2 */
}
/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

#ifndef TURN_OFF_BDT
/**
  * @brief USART10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART10_UART_Init(void)
{

  /* USER CODE BEGIN USART10_Init 0 */

  /* USER CODE END USART10_Init 0 */

  /* USER CODE BEGIN USART10_Init 1 */

  /* USER CODE END USART10_Init 1 */
  huart10.Instance = USART10;
  huart10.Init.BaudRate = 1843200;	//1843200;	//230,400	460800  921600  1843200(worked!)  3686400(doesn't work)
  huart10.Init.WordLength = UART_WORDLENGTH_8B;
  huart10.Init.StopBits = UART_STOPBITS_1;
  huart10.Init.Parity = UART_PARITY_NONE;
  huart10.Init.Mode = UART_MODE_TX_RX;
  huart10.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart10.Init.OverSampling = UART_OVERSAMPLING_16;
  huart10.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart10.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart10.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart10, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart10, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART10_Init 2 */

  /* USER CODE END USART10_Init 2 */

}

/**
  * @brief USB1_OTG_HS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB1_OTG_HS_PCD_Init(void)
{

  /* USER CODE BEGIN USB1_OTG_HS_Init 0 */

  /* USER CODE END USB1_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB1_OTG_HS_Init 1 */

  /* USER CODE END USB1_OTG_HS_Init 1 */
  hpcd_USB_OTG_HS1.Instance = USB1_OTG_HS;
  hpcd_USB_OTG_HS1.Init.dev_endpoints = 9;
  hpcd_USB_OTG_HS1.Init.speed = PCD_SPEED_HIGH;
  hpcd_USB_OTG_HS1.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hpcd_USB_OTG_HS1.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_HS1.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_HS1.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_HS1.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_HS1.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_HS1.Init.dma_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_HS1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB1_OTG_HS_Init 2 */

  /* USER CODE END USB1_OTG_HS_Init 2 */

}
/**
  * @brief XSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI1_Init(void)	//AP Memeory PSRAM
{

	  /* USER CODE BEGIN XSPI1_Init 0 */

	  /* USER CODE END XSPI1_Init 0 */

	  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

	  /* USER CODE BEGIN XSPI1_Init 1 */

	  /* USER CODE END XSPI1_Init 1 */
	  /* XSPI1 parameter configuration*/
	  hxspi1.Instance = XSPI1;
	  hxspi1.Init.FifoThresholdByte = 4;
	  hxspi1.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
	  hxspi1.Init.MemoryType = HAL_XSPI_MEMTYPE_APMEM_16BITS;
	  hxspi1.Init.MemorySize = HAL_XSPI_SIZE_256MB;
	  hxspi1.Init.ChipSelectHighTimeCycle = 5;
	  hxspi1.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
	  hxspi1.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
	  hxspi1.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
	  hxspi1.Init.ClockPrescaler = 1;
	  hxspi1.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
	  hxspi1.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;	//as ST's APS256XX setup: data hold for DTR writes
	  hxspi1.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_16KB;
	  hxspi1.Init.MaxTran = 0;
	  /* Release chip select every ~2us (as ST's setup) so the PSRAM can refresh: its CS-low time
	   * (tCEM) is limited to 4us. Long memory-mapped transfers otherwise hold CS low for up to
	   * 16KB. Counted in XSPI clocks: 2us at 75 MHz (IC3 /12, set in HAL_XSPI_MspInit, with
	   * the prescaler bypassed after init) = 150 cycles, minus 4 as in ST's formula.
	   * Recalculate if the IC3 divider changes. */
	  hxspi1.Init.Refresh = 146;
	  hxspi1.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
	  if (HAL_XSPI_Init(&hxspi1) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
	  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_1;
	  sXspiManagerCfg.Req2AckTime = 1;
	  if (HAL_XSPIM_Config(&hxspi1, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  /* USER CODE BEGIN XSPI1_Init 2 */

	  /* USER CODE END XSPI1_Init 2 */

	}

/**
  * @brief XSPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI2_Init(void)	//Macronix Flash
{

  /* USER CODE BEGIN XSPI2_Init 0 */

  /* USER CODE END XSPI2_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  /* USER CODE BEGIN XSPI2_Init 1 */

  /* USER CODE END XSPI2_Init 1 */
  /* XSPI2 parameter configuration*/
  hxspi2.Instance = XSPI2;
  hxspi2.Init.FifoThresholdByte = 1;
  hxspi2.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi2.Init.MemoryType = HAL_XSPI_MEMTYPE_MICRON;
  hxspi2.Init.MemorySize = HAL_XSPI_SIZE_16B;
  hxspi2.Init.ChipSelectHighTimeCycle = 1;
  hxspi2.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi2.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi2.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  hxspi2.Init.ClockPrescaler = 0;
  hxspi2.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi2.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
  hxspi2.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi2.Init.MaxTran = 0;
  hxspi2.Init.Refresh = 0;
  hxspi2.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi2) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_2;
  sXspiManagerCfg.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi2, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI2_Init 2 */

  /* USER CODE END XSPI2_Init 2 */

}
#endif

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOP_CLK_ENABLE();
  __HAL_RCC_GPIOO_CLK_ENABLE();
  __HAL_RCC_GPION_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure CAM_FLASH pin Output Level */
  HAL_GPIO_WritePin(CAM_FLASH_GPIO_Port, CAM_FLASH_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAM_PWRC_GPIO_Port, CAM_PWRC_Pin|TX_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : CAM_FLASH_Pin */
  GPIO_InitStruct.Pin = CAM_FLASH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAM_FLASH_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CAM_PWRC_Pin TX_EN_Pin */
  GPIO_InitStruct.Pin = CAM_PWRC_Pin|TX_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : NRST_CAM_Pin */
  GPIO_InitStruct.Pin = NRST_CAM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRST_CAM_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : WKUP2_Pin */
/*  GPIO_InitStruct.Pin = WKUP2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(WKUP2_GPIO_Port, &GPIO_InitStruct);*/

  /*Configure the EXTI line attribute */
  HAL_EXTI_ConfigLineAttributes(EXTI_LINE_2, EXTI_LINE_SEC);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /*Configure GPIO pins : CTS_FROM_SYSCNTR_Pin */
  GPIO_InitStruct.Pin = CTS_FROM_SYSCNTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CTS_FROM_SYSCNTR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : INTA_FRM_SYSCTL */
  GPIO_InitStruct.Pin = N6_WKUP_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(N6_WKUP_CLK_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : BIDIR INTS */
  GPIO_InitStruct.Pin = WKUP2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WKUP2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/*********************************************************/
/* LOOP                                                  */
/*********************************************************/
void LOOP (void){
uint32_t iters=0;
for(;;){
	HAL_UART_Transmit(&huart1, (unsigned char*)("PAUSE "), 6, 100);
	UARTshort(iters, 3);
	UARTreturn();
	iters++;
	HAL_Delay(5000);
	ledBlink(1);
}
}
/*********************************************************/
/* LED BLINK                                         */
/*********************************************************/
void ledBlink (uint8_t numBlinks){
uint8_t iters;
for(iters=numBlinks;iters>0;iters--){
//	  HAL_IWDG_Refresh(&hiwdg1);
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);//TP
	  HAL_Delay(50);
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);//TP
	  HAL_Delay(100);
}
}
/*********************************************************/
/* ENABLE CACHE (taken from example code for SDIO)       */
/*********************************************************/
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}
/***********************************************************************/
/*  send UART INT  											*/
/***********************************************************************/
void UARTint(uint32_t x){
	UARTshort(x>>16, 4);
	UARTshort((x&0x0000FFFF), 4);
}
/***********************************************************************/
/*  send UART int												*/
/***********************************************************************/
 void UARTshort(uint16_t a, uint8_t d){
 	uint8_t iters,senduint8_ts[4];
	short z,zz;
	iters=d;
	z=hex2ToAscii((a>>8)&0x00FF);
	zz=hex2ToAscii(a&0x00FF);
	senduint8_ts[d-1]=(uint8_t)zz;
	if(--iters>0)senduint8_ts[d-2]=(uint8_t)(zz>>8);
	if(--iters>0)senduint8_ts[d-3]=(uint8_t)(z);
	if(--iters>0)senduint8_ts[d-4]=(uint8_t)(z>>8);
	HAL_UART_Transmit(&huart1, (uint8_t*)senduint8_ts, d, 100);
}
 /***********************************************************************/
 /*  send UART space         	  										*/
 /***********************************************************************/
  void UARTspace(void){
 	HAL_UART_Transmit(&huart1, (uint8_t*)(" "), 1, 100);
 }
/************************************************************************/
/*  send UART space           											*/
/************************************************************************/
void UARTreturn(void){
	HAL_UART_Transmit(&huart1, (uint8_t*)("\r\n"), 2, 100);
}
/************************************************************************/
/*  hex2ToAscii 2 bytes hex to 4 bytes Ascii							*/
/************************************************************************/
uint32_t hex2ToAscii(uint32_t hexx)
{
    int a, b, c, d;
    hexx &= 0x0000FFFF;
    a = (hex1ToAscii(hexx));
    hexx >>= 4;
    b = (hex1ToAscii(hexx));
    b <<= 8;
    hexx >>= 4;
    c = (hex1ToAscii(hexx));
    c <<= 16;
    hexx >>= 4;
    d = (hex1ToAscii(hexx));
    d <<= 24;
    return a + b + c + d;
}
/************************************************************************/
/*  hex1ToAscii nibble to 1-byte Ascii  								*/
/************************************************************************/
uint8_t hex1ToAscii(uint8_t hex)
{
    int a;
    hex &= 0x0F;
    switch (hex)
    {
    case 0:
        a = 0x30;
        break;
    case 1:
        a = 0x31;
        break;
    case 2:
        a = 0x32;
        break;
    case 3:
        a = 0x33;
        break;
    case 4:
        a = 0x34;
        break;
    case 5:
        a = 0x35;
        break;
    case 6:
        a = 0x36;
        break;
    case 7:
        a = 0x37;
        break;
    case 8:
        a = 0x38;
        break;
    case 9:
        a = 0x39;
        break;
    case 10:
        a = 0x41;
        break;
    case 11:
        a = 0x42;
        break;
    case 12:
        a = 0x43;
        break;
    case 13:
        a = 0x44;
        break;
    case 14:
        a = 0x45;
        break;
    case 15:
        a = 0x46;
        break;
    };
    return a;
}
/***********************************************************************/
/*  PSRAM TEST  */
/***********************************************************************/

uint8_t stm_xspi_psram_test(uint8_t exhaustive) {

	uint8_t const pattern = 0xaa;
	uint8_t const antipattern = 0x55;
	volatile uint8_t *const mem_base = (uint8_t *) 0x90000000;

	char error_buffer[1024];
	uint32_t mem_size = 0x2000000;

	#if (__DCACHE_PRESENT == 1)
	uint8_t i_cache_disabled = 0;
	uint8_t d_cache_disabled = 0;

	// Disable caches for testing.
	if (SCB->CCR & (uint32_t) SCB_CCR_IC_Msk) {
		SCB_DisableICache();
		i_cache_disabled = 1;
	}

	if (SCB->CCR & (uint32_t) SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		d_cache_disabled = 1;
	}
	#endif

    return 1;
}
/***********************************************************************/
/*  PRINT SRAM BUFFER           									   */
/***********************************************************************/
void printDestBuff(void){
	uint32_t iters;
	UARTreturn();
	for(iters=0;iters<921603;iters+=50001){
		UARTint(iters);
		UARTspace();
		UARTshort(Main_DestBuffer[iters], 2);
		UARTshort(Main_DestBuffer[iters+1], 2);
		UARTshort(Main_DestBuffer[iters+2], 2);
		UARTreturn();
	}
	UARTint(921598);
	UARTspace();
	UARTshort(Main_DestBuffer[921596], 2);
	UARTshort(Main_DestBuffer[921597], 2);
	UARTshort(Main_DestBuffer[921598], 2);
	UARTshort(Main_DestBuffer[921599], 2);
	UARTreturn();
	UARTint(921600);
	UARTspace();
	UARTshort(Main_DestBuffer[921600], 2);
	UARTshort(Main_DestBuffer[921601], 2);
	UARTshort(Main_DestBuffer[921602], 2);
	UARTshort(Main_DestBuffer[921603], 2);

	UARTreturn();
}
/***********************************************************************/
/*  CLEAR SRAM BUFFER           									   */
/***********************************************************************/
void clearDestBuff(void){
	uint32_t iters;
	uint8_t dum= 0x00;//'B';
	for(iters=0;iters<120000;iters++){
		Main_DestBuffer[iters] = dum;
	}
	Main_DestBuffer[100]=0;
	Main_DestBuffer[200]=0;
	Main_DestBuffer[500]=0;
}
/***********************************************************************/
/*  PRINT PSRAM DUMP BUFFER           								   */
/***********************************************************************/
void printDumpBuff(void){
	uint32_t iters;

	for(iters=0;iters<600;iters+=3){
//		UARTshort(Dump_DestBuffer[iters], 2);
//		UARTshort(Dump_DestBuffer[iters+1], 2);
//		UARTshort(Dump_DestBuffer[iters+2], 2);
		UARTspace();
		if((iters%1500)==0){
			UARTreturn();
			UARTshort(iters>>16, 4);
			UARTshort(iters&0x0000FFFF, 4);
			UARTspace();
			UARTreturn();
		}
	}
	UARTreturn();
}
/***********************************************************************/
/*  CLEAR PSRAM DUMP BUFFER           								   */
/***********************************************************************/
void clearDumpBuff(void){
	uint32_t iters;
	uint8_t dum=2;

	for(iters=0;iters<120000;iters++){
//		Dump_DestBuffer[iters] = dum;
	}

}
/***********************************************************************/
/*  PSRAM TEST               										   */
/***********************************************************************/
/*uint8_t psramTest(uint8_t exhaustive){
	uint16_t errorBuffer = 0;
	uint32_t index, index_K, iters, Kcount;
	uint32_t const pattern = 0xAA55AA55;
	uint32_t const antipattern = 0x55AA55AA;
	volatile uint8_t *const mem_base = (uint32_t *) 0x90000000;
	uint32_t mem_size = 0x2000000;

	#if (__DCACHE_PRESENT == 1)
	uint8_t i_cache_disabled = 0;
	uint8_t d_cache_disabled = 0;

	// Disable caches for testing.
	if (SCB->CCR & (uint32_t) SCB_CCR_IC_Msk) {
		SCB_DisableICache();
		i_cache_disabled = 1;
	}

	if (SCB->CCR & (uint32_t) SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		d_cache_disabled = 1;
	}
	#endif

	if (HAL_XSPI_MemoryMapped(&hxspi1, &sMemMappedCfg) != HAL_OK)
	{
	  Error_Handler();
	}

    // Test data bus
    for (uint32_t i = 0; i < 16; i++) {
    	*((volatile uint32_t *) mem_base) = 0x12345678;	//(1 << i);
        __DSB();
        if (*((volatile uint32_t *) mem_base) != (0x12345678 *//*1 << i*//*)) {
  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS ERROR\r\n"), 22, 100);
          return 0;
        }else{
  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS GOOD\r\n"), 21, 100);
      }
    }
    // Test address bus
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        mem_base[i] = pattern;
        __DSB();
        if (mem_base[i] != pattern) {
			UARTshort(i,4);
    		HAL_UART_Transmit(&huart1, (unsigned char*)(" PSRAM ADDRESS BUS ERROR\r\n"), 26, 100);
            return 0;
        }else{
  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDRESS BUS GOOD\r\n"), 25, 100);
      }
    }
    // Check for aliasing (overlapping addresses)
    mem_base[0] = antipattern;
    __DSB();
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        if (mem_base[i] != pattern) {
    		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ALIAS ERROR\r\n"), 19, 100);
            return 0;
        }
    }
    mem_size = 256 * 1024;
    // Test all RAM cells
    Kcount=0;
    if (exhaustive) {
        // Write all memory first then compare, so even if the cache
        // is enabled, it's not just writing and reading from cache.
        // Note: This test should also detect refresh rate issues.
        for (uint32_t i = 0; i < mem_size; i++) {
            mem_base[i] = ((i % 2) ? pattern : antipattern);
        }

        for (uint32_t i = 0; i < mem_size; i++) {
            if (mem_base[i] != ((i % 2) ? pattern : antipattern)) {
        		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM FAIL EXHAUSTIVE\r\n"), 23, 100);
                return 0;
            }else{
            	if((i%1024)==0){
            		HAL_UART_Transmit(&huart1, (unsigned char*)("."), 1, 100);
                	Kcount++;
            	}
            }
        }
        UARTreturn();
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0x"), 2, 100);
        UARTshort(Kcount, 3);
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0000 PSRAM GOOD\r\n"), 17, 100);
    }else HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM GOOD\r\n"), 12, 100);
    #if (__DCACHE_PRESENT == 1)
    // Re-enable caches if they were enabled before the test started.
    if (i_cache_disabled) {
        SCB_EnableICache();
    }
    if (d_cache_disabled) {
        SCB_EnableDCache();
    }
    #endif
    return(1);
}*/
/***********************************************************************/
/*  PSRAM ADDRESS-AS-DATA TEST                                          */
/*  Writes every 32-bit word with its own address, sequentially (so the */
/*  XSPI merges the writes into long transfers, like the camera does),  */
/*  then reads it all back; then repeats with the inverted value.       */
/*  Catches data landing at the wrong address (burst wrap, crossed      */
/*  address lines, aliasing) and stuck data bits, which a repeating     */
/*  pattern cannot. Prints the error count and first mismatch.          */
/*  Returns the number of mismatched words (0 = pass).                  */
/***********************************************************************/
uint32_t psramAddrTest(uint32_t *base, uint32_t numBytes){
	volatile uint32_t *mem = (volatile uint32_t *) base;
	uint32_t numWords = numBytes / 4;
	uint32_t errors = 0;
	uint32_t firstAddr = 0, firstExpected = 0, firstGot = 0;
	uint32_t pass, iters, expected, got, startTick;

	HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDR TEST @"), 17, 100);
	UARTint((uint32_t) base);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" BYTES "), 7, 100);
	UARTint(numBytes);
	UARTreturn();
	startTick = HAL_GetTick();

	for(pass=0; pass<2; pass++){
		uint32_t invert = (pass == 0) ? 0 : 0xFFFFFFFF;
		for(iters=0; iters<numWords; iters++){
			mem[iters] = ((uint32_t) &mem[iters]) ^ invert;
		}
		__DSB();
		for(iters=0; iters<numWords; iters++){
			expected = ((uint32_t) &mem[iters]) ^ invert;
			got = mem[iters];
			if(got != expected){
				if(errors == 0){
					firstAddr = (uint32_t) &mem[iters];
					firstExpected = expected;
					firstGot = got;
				}
				errors++;
			}
		}
	}

	HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDR TEST ERRORS "), 23, 100);
	UARTint(errors);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" MS "), 4, 100);
	UARTint(HAL_GetTick() - startTick);
	UARTreturn();
	if(errors != 0){
		/* If 'got' is another word's address, that's where this word's data ended up */
		HAL_UART_Transmit(&huart1, (unsigned char*)("FIRST BAD @"), 11, 100);
		UARTint(firstAddr);
		HAL_UART_Transmit(&huart1, (unsigned char*)(" EXPECTED "), 10, 100);
		UARTint(firstExpected);
		HAL_UART_Transmit(&huart1, (unsigned char*)(" GOT "), 5, 100);
		UARTint(firstGot);
		UARTreturn();
	}
	return errors;
}
uint8_t psramTest(uint8_t exhaustive, uint8_t firstTime){
	uint16_t errorBuffer = 0;
	uint32_t index, index_K, iters, Kcount;
//	uint32_t const pattern = 0xAA55AA55;	//for address test
//	uint32_t const antipattern = 0x55AA55AA;	//for address test
	uint8_t const pattern = 0xaa;
	uint8_t const antipattern = 0x55;
	volatile uint8_t *const mem_base = (uint32_t *) 0x90000000;
	uint32_t mem_size = 0x2000000;

	#if (__DCACHE_PRESENT == 1)
	uint8_t i_cache_disabled = 0;
	uint8_t d_cache_disabled = 0;

	// Disable caches for testing.
	if (SCB->CCR & (uint32_t) SCB_CCR_IC_Msk) {
		SCB_DisableICache();
		i_cache_disabled = 1;
	}

	if (SCB->CCR & (uint32_t) SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		d_cache_disabled = 1;
	}
	#endif

if(firstTime==0){	//this fails if run psramTest more than once
	if (HAL_XSPI_MemoryMapped(&hxspi1, &sMemMappedCfg) != HAL_OK)
	{
	  Error_Handler();
	}
}
    // Test data bus
    for (uint32_t i = 0; i < 16; i++) {
    	*((volatile uint32_t *) mem_base) = 0x12345678;	//(1 << i);
        __DSB();
        if (*((volatile uint32_t *) mem_base) != (0x12345678 /*1 << i*/)) {
  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS ERROR\r\n"), 22, 100);
          return 0;
        }else{
//  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS GOOD\r\n"), 21, 100);
      }
    }
    // Test address bus
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        mem_base[i] = pattern;
        __DSB();
        if (mem_base[i] != pattern) {
			UARTshort(i,4);
    		HAL_UART_Transmit(&huart1, (unsigned char*)(" PSRAM ADDRESS BUS ERROR\r\n"), 26, 100);
            return 0;
        }else{
//  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDRESS BUS GOOD\r\n"), 25, 100);
      }
    }
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        mem_base[i] = pattern;
        __DSB();
        if (mem_base[i] != pattern) {
			UARTshort(i,4);
    		HAL_UART_Transmit(&huart1, (unsigned char*)(" PSRAM ADDRESS BUS ERROR\r\n"), 26, 100);
            return 0;
        }else{
//  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDRESS BUS GOOD\r\n"), 25, 100);
      }
    }
    // Check for aliasing (overlapping addresses)
    mem_base[0] = antipattern;
    __DSB();
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        if (mem_base[i] != pattern) {
    		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ALIAS ERROR\r\n"), 19, 100);
            return 0;
        }
    }
    mem_size = 256 * 1024;
    // Test all RAM cells
    Kcount=0;
    if (exhaustive) {
        // Write all memory first then compare, so even if the cache
        // is enabled, it's not just writing and reading from cache.
        // Note: This test should also detect refresh rate issues.
        for (uint32_t i = 0; i < mem_size; i++) {
            mem_base[i] = ((i % 2) ? pattern : antipattern);
        }

        for (uint32_t i = 0; i < mem_size; i++) {
            if (mem_base[i] != ((i % 2) ? pattern : antipattern)) {
        		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM FAIL EXHAUSTIVE\r\n"), 23, 100);
                return 0;
            }else{
            	if((i%1024)==0){
            		HAL_UART_Transmit(&huart1, (unsigned char*)("."), 1, 100);
                	Kcount++;
            	}
            }
        }
        UARTreturn();
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0x"), 2, 100);
        UARTshort(Kcount, 3);
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0000 PSRAM GOOD\r\n"), 17, 100);
    }else HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM GOOD\r\n"), 12, 100);
    #if (__DCACHE_PRESENT == 1)
    // Re-enable caches if they were enabled before the test started.
    if (i_cache_disabled) {
        SCB_EnableICache();
    }
    if (d_cache_disabled) {
        SCB_EnableDCache();
    }
    #endif
    return(1);
}

/***********************************************************************/
/*  PSRAM TEST 32 BIT               										   */
/***********************************************************************/
/*uint8_t psramTest32Bit(uint8_t exhaustive){
	uint16_t errorBuffer = 0;
	uint32_t index, index_K, iters, Kcount;
	uint32_t const pattern = 0xAA55AA55;
	uint32_t const antipattern = 0x55AA55AA;
	volatile uint8_t *const mem_base = (uint8_t *) 0x90000000;
	uint32_t mem_size = 0x2000000;

	#if (__DCACHE_PRESENT == 1)
	uint8_t i_cache_disabled = 0;
	uint8_t d_cache_disabled = 0;

	// Disable caches for testing.
	if (SCB->CCR & (uint32_t) SCB_CCR_IC_Msk) {
		SCB_DisableICache();
		i_cache_disabled = 1;
	}

	if (SCB->CCR & (uint32_t) SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		d_cache_disabled = 1;
	}
	#endif

	if (HAL_XSPI_MemoryMapped(&hxspi1, &sMemMappedCfg) != HAL_OK)
	{
	  Error_Handler();
	}

    // Test data bus
    for (uint32_t i = 0; i < 16; i++) {
    	*((volatile uint32_t *) mem_base) = (1 << i);
        __DSB();
        if (*((volatile uint32_t *) mem_base) != (1 << i)) {
  		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS ERROR\r\n"), 22, 100);
          return 0;
        }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM DATA BUS GOOD\r\n"), 21, 100);
    }
    // Test address bus
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        mem_base[i] = pattern;
        __DSB();
        if (mem_base[i] != pattern) {
			UARTshort(i,4);
    		HAL_UART_Transmit(&huart1, (unsigned char*)(" PSRAM ADDRESS BUS ERROR\r\n"), 26, 100);
            return 0;
        }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ADDRESS BUS GOOD\r\n"), 25, 100);
    }
    }
    // Check for aliasing (overlapping addresses)
    mem_base[0] = antipattern;
    __DSB();
    for (uint32_t i = 1; i < mem_size; i <<= 1) {
        if (mem_base[i] != pattern) {
    		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ALIAS ERROR\r\n"), 19, 100);
            return 0;
        }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM ALIAS GOOD\r\n"), 18, 100);
    }
    mem_size = 256 * 1024;
    // Test all RAM cells
    Kcount=0;
    if (exhaustive) {
        // Write all memory first then compare, so even if the cache
        // is enabled, it's not just writing and reading from cache.
        // Note: This test should also detect refresh rate issues.
        for (uint32_t i = 0; i < mem_size; i++) {
            mem_base[i] = ((i % 2) ? pattern : antipattern);
        }

        for (uint32_t i = 0; i < mem_size; i++) {
            if (mem_base[i] != ((i % 2) ? pattern : antipattern)) {
        		HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM FAIL EXHAUSTIVE\r\n"), 23, 100);
                return 0;
            }else{
            	if((i%1024)==0){
            		HAL_UART_Transmit(&huart1, (unsigned char*)("."), 1, 100);
                	Kcount++;
            	}
            }
        }
        UARTreturn();
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0x"), 2, 100);
        UARTshort(Kcount, 3);
    	HAL_UART_Transmit(&huart1, (unsigned char*)("0000 PSRAM GOOD\r\n"), 17, 100);
    }else HAL_UART_Transmit(&huart1, (unsigned char*)("PSRAM GOOD\r\n"), 12, 100);
    #if (__DCACHE_PRESENT == 1)
    // Re-enable caches if they were enabled before the test started.
    if (i_cache_disabled) {
        SCB_EnableICache();
    }
    if (d_cache_disabled) {
        SCB_EnableDCache();
    }
    #endif
    return(1);
}*/
/***********************************************************************/
/*  SEND COLOR PIC            										   */
/***********************************************************************/
void sendPic(void){
	#define CRC_SIZE 1200
	uint32_t iters, packIters, imgIters;
	uint8_t picPacket[1212];	//1208 for preambles, 1212 for preambles plus CRCs and pack#s.
	uint16_t crcrc;
	uint8_t* destBufferPtr = Main_DestBuffer;
/*	picPacket[0]=0x55;
	picPacket[1]=0x55;
	picPacket[2]=0x55;
	picPacket[3]=0x55;
	picPacket[4]=0x55;
	picPacket[5]=0x55;
	picPacket[6]=0x55;
	picPacket[7]=0xD5;*/

	imgIters = 0xAA;	//TSHOOOOOOOOT added this line as a flag to recognize which byte is which at the SysCntlr.

	/* Each packet is 1200 image bytes in picPacket[0..1199], exactly what is transmitted.
	 * (Filling from [8] left 8 uninitialized bytes at the start of every packet and shifted
	 * the image, rotating the RGB order every packet: a checkerboard of wrong colours.) */
#ifdef RESOLUTION_2400_1600
	for(packIters=0;packIters<IMAGE_PACKETS;packIters++){	//IMAGE_PACKETS x 1200 = IMAGE_WIDTH x IMAGE_HEIGHT x 3
		for(iters=0;iters<1200;iters++){
			picPacket[iters]=*destBufferPtr++;
		}
/*		for(packIters=0;packIters<9600;packIters++){	//9600 for 2400x1600
			for(iters=8;iters<1208;iters++){	//8 to 1208 for preambles.
				picPacket[iters]=*destBufferPtr++;
			}*/
#else
	for(packIters=0;packIters<768;packIters++){	//768 x 1200 = 640x480x3
		for(iters=0;iters<1200;iters++){
			picPacket[iters]=*destBufferPtr++;
		}
#endif
		HAL_UART_Transmit(&huart10,picPacket,1200,100);//1200 without CRCs. was 1212 with CRCs and preamble	//115,200

/*		crcrc=calcCRC(&picPacket[8], CRC_SIZE);//CRC_SIZE);

		picPacket[1208]=crcrc>>8;
		picPacket[1209]=crcrc&0x00FF;
		picPacket[1210]=imgIters;
		picPacket[1211]=packIters;
		HAL_UART_Transmit(&huart10,picPacket,1212,100);	//1212 will include CRCs and pack#s.*/
//		UARTshort(picPacket[6],2);
//		UARTshort(picPacket[7],2);
//		UARTspace();
//		UARTshort(picPacket[8],2);
//		UARTshort(picPacket[9],2);
//		UARTshort(picPacket[10],2);
//		UARTshort(picPacket[11],2);
//		UARTspace();
//		HAL_UART_Transmit(&huart1, (unsigned char*)("."), 1, 100);

//		while ((HAL_GPIO_ReadPin(CTS_FROM_SYSCNTR_GPIO_Port, CTS_FROM_SYSCNTR_Pin))==1){}
//		while ((HAL_GPIO_ReadPin(CTS_FROM_SYSCNTR_GPIO_Port, CTS_FROM_SYSCNTR_Pin))==0){}

	}


	HAL_UART_Transmit(&huart1, (unsigned char*)("SENT PIC\r\n"), 10, 100);


}
/***********************************************************************/
/*  SEND 3 GRAYSCALE PICS           										   */
/***********************************************************************/
void send3Grays(void){
	#define CRC_SIZE 1200
	uint32_t iters, packIters, imgIters;
	uint8_t picPacket[1200];	//was 1208 for CRCs
//	uint16_t crcrc;
//	uint8_t* destBufferPtr = grayscale_A;
	uint8_t* destBufferPtr = Main_DestBuffer;
/*	picPacket[0]=0x55;
	picPacket[1]=0x55;
	picPacket[2]=0x55;
	picPacket[3]=0x55;
	picPacket[4]=0x55;
	picPacket[5]=0x55;
	picPacket[6]=0x55;
	picPacket[7]=0xD5;*/
	for(packIters=0;packIters<768;packIters++){	//1
		for(iters=0;iters<1200;iters++){
			picPacket[iters]=*destBufferPtr++;
		}
/*		crcrc=calcCRC(&picPacket[8], CRC_SIZE);
		picPacket[1208]=crcrc>>8;
		picPacket[1209]=crcrc&0x00FF;
		picPacket[1210]=imgIters;
		picPacket[1211]=packIters;*/
	/*	HAL_UART_Transmit(&huart1,picPacket,20,100);
		UARTshort(crcrc,4);
		UARTreturn();*/
		HAL_UART_Transmit(&huart10,picPacket,1200,100);	//1212 for CRCs
	}

	HAL_UART_Transmit(&huart1, (unsigned char*)("SENT PIC\r\n"), 10, 100);
}
/***********************************************************************/
/*  SEND COLOR BARS            										   */
/***********************************************************************/
void sendColorBars(void){
	uint32_t iters, packIters, imgIters;
	uint8_t pixVal[3];
	uint32_t numPacks;
//	uint8_t startFrame[];

//#define PIXELS_PER_BAR 48000	//for 800x480 to cover full LCD	(bar is 60 pix high, x 800 wide = 48000)
#define PIXELS_PER_BAR 38400	//for 640x480 to simulate N6cam	(bar is 60 pix high, x 640 wide = 38400)
#define PACK_DELAY (10)

	pixVal[0]=1;pixVal[1]=2;pixVal[2]=3;
//	for(iters=0;iters<PIXELS_PER_BAR;iters++){				//48000 for
//		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
//	}

uint8_t picPacket[1200];	//was 1212 with CRCs
//uint16_t crcrc;
//#define CRC_SIZE 1200
//picPacket[0]=0x55;
//picPacket[1]=0x55;
//picPacket[2]=0x55;
//picPacket[3]=0x55;
//picPacket[4]=0x55;
//picPacket[5]=0x55;
//picPacket[6]=0x55;
//picPacket[7]=0xD5;
#ifdef RESOLUTION_2400_1600
	numPacks=IMAGE_PACKETS/8;	//packets per colour bar
#else
	numPacks=96;
#endif
	for(packIters=0;packIters<numPacks;packIters++){	//1
	for(iters=0;iters<1200;iters+=3){//WAS 8..1208 WITH PREAMBLE
		picPacket[iters]=0xFF;
		picPacket[iters+1]=0x00;
		picPacket[iters+2]=0x00;
	}

//	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
//	picPacket[1208]=crcrc>>8;
//	picPacket[1209]=crcrc&0x00FF;
//	picPacket[1210]=imgIters;
//	picPacket[1211]=packIters;
	HAL_UART_Transmit(&huart10,picPacket,1200,100);//was 1212 with CRCs and preamble	//115,200
}
for(packIters=0;packIters<numPacks;packIters++){	//2
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0x00;
		picPacket[iters+1]=0xFF;
		picPacket[iters+2]=0x00;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//230,400
}
for(packIters=0;packIters<numPacks;packIters++){	//3
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0x00;
		picPacket[iters+1]=0x00;
		picPacket[iters+2]=0xFF;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//345,600
}
for(packIters=0;packIters<numPacks;packIters++){	//4
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0xFF;
		picPacket[iters+1]=0xFF;
		picPacket[iters+2]=0x00;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//460,800
}
for(packIters=0;packIters<numPacks;packIters++){	//5
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0x00;
		picPacket[iters+1]=0x00;
		picPacket[iters+2]=0x00;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//576,000
}
for(packIters=0;packIters<numPacks;packIters++){	//6
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0xFF;
		picPacket[iters+1]=0xFF;
		picPacket[iters+2]=0xFF;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//691,200
}
for(packIters=0;packIters<numPacks;packIters++){	//7
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0xFF;
		picPacket[iters+1]=0x00;
		picPacket[iters+2]=0xFF;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//806,400
}
for(packIters=0;packIters<numPacks;packIters++){	//8
	for(iters=0;iters<1200;iters+=3){
		picPacket[iters]=0x00;
		picPacket[iters+1]=0xFF;
		picPacket[iters+2]=0xFF;
	}
/*	crcrc=calcCRC(&picPacket[8], CRC_SIZE);
	picPacket[1208]=crcrc>>8;
	picPacket[1209]=crcrc&0x00FF;
	picPacket[1210]=imgIters;
	picPacket[1211]=packIters;*/
	HAL_UART_Transmit(&huart10,picPacket,1200,100);	//921,600
}
//		HAL_Delay(PACK_DELAY);

/*	pixVal[0]=0;pixVal[1]=255;pixVal[2]=0;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=0;pixVal[1]=255;pixVal[2]=255;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=255;pixVal[1]=255;pixVal[2]=0;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=0;pixVal[1]=255;pixVal[2]=255;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=255;pixVal[1]=0;pixVal[2]=0;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=0;pixVal[1]=0;pixVal[2]=255;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);
	}
	pixVal[0]=255;pixVal[1]=255;pixVal[2]=0;
	for(iters=0;iters<PIXELS_PER_BAR;iters++){
		HAL_UART_Transmit(&huart10, pixVal, 3, 100);

	}*/
	HAL_UART_Transmit(&huart1, (unsigned char*)("DONE BARS\r\n"), 11, 100);
}
/***********************************************************************/
/*  STORE COLOR BARS IN PSRAM          										   */
/***********************************************************************/
void storeColorBars(void){
	uint32_t iters, packIters, imgIters;
	uint8_t pixVal[3];
	uint32_t numPacks;
	uint8_t* destBufferPtr = Main_DestBuffer;

//#define PIXELS_PER_BAR 48000	//for 800x480 to cover full LCD	(bar is 60 pix high, x 800 wide = 48000)
#define PIXELS_PER_BAR 38400	//for 640x480 to simulate N6cam	(bar is 60 pix high, x 640 wide = 38400)
#define PACK_DELAY (10)

	pixVal[0]=1;pixVal[1]=2;pixVal[2]=3;

uint8_t picPacket[1200];	//was 1212 with CRCs
#ifdef RESOLUTION_2400_1600
	numPacks=IMAGE_PACKETS/8;	//packets per colour bar
#else
	numPacks=96;
#endif
	for(packIters=0;packIters<numPacks;packIters++){	//1
	for(iters=0;iters<1200;iters+=3){//WAS 8..1208 WITH PREAMBLE
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0x00;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//2
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0x00;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//3
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0xFF;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//4
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0x00;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//5
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0x00;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//6
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0xFF;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//7
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0xFF;
	}
}
for(packIters=0;packIters<numPacks;packIters++){	//8
	for(iters=0;iters<1200;iters+=3){
		*destBufferPtr++ = 0x00;
		*destBufferPtr++ = 0xFF;
		*destBufferPtr++ = 0xFF;
	}
}
	HAL_UART_Transmit(&huart1, (unsigned char*)("DONE BARS\r\n"), 11, 100);
}

/***********************************************************************/
/**  PSRAM WRITE REGISTER
* @brief  Write mode register
* @param  Ctx Component object pointer
* @param  Address Register address
* @param  Value Register value pointer
* @retval error status
*/
/***********************************************************************/
/***********************************************************************/
uint32_t APS256_WriteReg(XSPI_HandleTypeDef *Ctx, uint32_t Address, uint8_t *Value)
{
  XSPI_RegularCmdTypeDef sCommand1={0};

  /* Initialize the write register command */
  sCommand1.OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand1.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
  sCommand1.InstructionWidth    = HAL_XSPI_INSTRUCTION_8_BITS;
  sCommand1.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
  sCommand1.Instruction        = WRITE_REG_CMD;
  sCommand1.AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
  sCommand1.AddressWidth        = HAL_XSPI_ADDRESS_32_BITS;
  sCommand1.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
  sCommand1.Address            = Address;
  sCommand1.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand1.DataMode           = HAL_XSPI_DATA_8_LINES;
  sCommand1.DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
  sCommand1.DataLength         = 2;
  sCommand1.DummyCycles        = 0;
  sCommand1.DQSMode            = HAL_XSPI_DQS_DISABLE;

  /* Configure the command */
  if (HAL_XSPI_Command(Ctx, &sCommand1, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Transmission of the data */
  if (HAL_XSPI_Transmit(Ctx, (uint8_t *)(Value), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/***********************************************************************/
/**	PSRAM READ REGISTER
* @brief  Read mode register value
* @param  Ctx Component object pointer
* @param  Address Register address
* @param  Value Register value pointer
* @param  LatencyCode Latency used for the access
* @retval error status
*/
/***********************************************************************/
uint32_t APS256_ReadReg(XSPI_HandleTypeDef *Ctx, uint32_t Address, uint8_t *Value, uint32_t LatencyCode)
{
  XSPI_RegularCmdTypeDef sCommand={0};

  /* Initialize the read register command */
  sCommand.OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
  sCommand.InstructionWidth    = HAL_XSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Instruction        = READ_REG_CMD;
  sCommand.AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
  sCommand.AddressWidth        = HAL_XSPI_ADDRESS_32_BITS;
  sCommand.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
  sCommand.Address            = Address;
  sCommand.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand.DataMode           = HAL_XSPI_DATA_8_LINES;
  sCommand.DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
  sCommand.DataLength            = 2;
  sCommand.DummyCycles        = (LatencyCode - 1U);
  sCommand.DQSMode            = HAL_XSPI_DQS_ENABLE;

  /* Configure the command */
  if (HAL_XSPI_Command(Ctx, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Reception of the data */
  if (HAL_XSPI_Receive(Ctx, (uint8_t *)Value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}


/***********************************************************************/
/** PSRAM CONFIG
* @brief  Switch from Octal Mode to Hexa Mode on the memory
* @param  None
* @retval None
*/
/***********************************************************************/
static void Configure_APMemory(void)
{
	  /* MR0 register for read and write */
	  uint8_t regW_MR0[2]={0x30,0x8D}; /* To configure AP memory Latency Type and drive Strength */
	  uint8_t regR_MR0[2]={0};

	  uint8_t regW_MR4[2]={0x20,0xF0}; /* To configure AP memory, Write Latency=7 up to 200MHz */
	  uint8_t regR_MR4[2]={0};

	  /* MR8 register for read and write */
	  uint8_t regW_MR8[2]={0x4B,0x08}; /* To configure AP memory Burst Type */
	  uint8_t regR_MR8[2]={0};

	  /*Read Latency */
	  uint8_t latency=6;

	  /* Configure Read Latency and drive Strength */
	  if (APS256_WriteReg(&hxspi1, MR0, regW_MR0) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Check MR0 configuration */
	  if (APS256_ReadReg(&hxspi1, MR0, regR_MR0, latency ) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Check MR0 configuration */
	  if (regR_MR0 [0] != regW_MR0 [0])
	  {
	    Error_Handler() ;
	  }

	  /* Configure Write Latency */
	  if (APS256_WriteReg(&hxspi1, MR4, regW_MR4) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  /* Check MR4 configuration */
	  if (APS256_ReadReg(&hxspi1, MR4, regR_MR4, latency) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if (regR_MR4[0] != regW_MR4[0])
	  {
	    Error_Handler() ;
	  }

	  /* Configure Burst Length */
	  if (APS256_WriteReg(&hxspi1, MR8, regW_MR8) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Check MR8 configuration */
	  if (APS256_ReadReg(&hxspi1, MR8, regR_MR8, 6) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  if (regR_MR8[0] != regW_MR8[0])
	  {
	    Error_Handler() ;
	  }
	}
/***********************************************************************/
/*  PSRAM INIT BASED ON MX  										   */
/***********************************************************************/
void psramInitFrmMX(void){
	  XSPI_RegularCmdTypeDef sCommand = {0};

	  Configure_APMemory();

	  /* Bypass the Pre-scaler */
	  HAL_XSPI_SetClockPrescaler(&hxspi1, 0);// change, XSPI1/PSRAM CLK: 200MHz
	  /*Configure Memory Mapped mode*/

	  sCommand.OperationType      = HAL_XSPI_OPTYPE_WRITE_CFG;
	  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
	  sCommand.InstructionWidth   = HAL_XSPI_INSTRUCTION_8_BITS;
	  sCommand.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	  /* Linear burst commands, as in ST's APS256XX driver. The plain 0x80/0x00 commands are
	   * wrapped bursts (2KB wrap with MR8 = 0x4B): a memory-mapped transfer that crosses a 2KB
	   * boundary wraps back to the start of that 2KB block and lands at the wrong address. */
	  sCommand.Instruction        = WRITE_LINEAR_BURST_CMD;
	  sCommand.AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
	  sCommand.AddressWidth       = HAL_XSPI_ADDRESS_32_BITS;
	  sCommand.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
	  sCommand.Address            = 0x0;
	  sCommand.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	  sCommand.DataMode           = HAL_XSPI_DATA_16_LINES;
	  sCommand.DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
	  sCommand.DataLength         = BUFFERSIZE;
	  sCommand.DummyCycles        = DUMMY_CLOCK_CYCLES_WRITE;
	  sCommand.DQSMode            = HAL_XSPI_DQS_ENABLE;

	  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  sCommand.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
	  sCommand.Instruction = READ_LINEAR_BURST_CMD;
	  sCommand.DummyCycles = DUMMY_CLOCK_CYCLES_READ;
	  sCommand.DQSMode     = HAL_XSPI_DQS_ENABLE;

	  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  sMemMappedCfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_ENABLE;
	  sMemMappedCfg.TimeoutPeriodClock      = 0x34;
	}
/**
  * @brief  ISP Middleware helper. Camera sensor info getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
  IMX335_SensorInfo_t info;

  UNUSED(Instance);

  /* Copy field by field: the BSP IMX335 driver uses the ISP <= 1.2.x layout (no again_max),
   * while the ISP library >= 1.3.0 inserts again_max before exposure_min. */
  if (IMX335_GetSensorInfo(&IMX335Obj, &info) != IMX335_OK)
  {
    return ISP_ERR_SENSORINFO;
  }

  strncpy(SensorInfo->name, info.name, sizeof(SensorInfo->name) - 1);
  SensorInfo->name[sizeof(SensorInfo->name) - 1] = '\0';
  SensorInfo->bayer_pattern = info.bayer_pattern;
  SensorInfo->color_depth = info.color_depth;
  SensorInfo->width = info.width;
  SensorInfo->height = info.height;
  SensorInfo->gain_min = info.gain_min;
  SensorInfo->gain_max = info.gain_max;
  SensorInfo->again_max = 30000;  /* IMX335 max analog gain: 30 dB, in mdB */
  SensorInfo->exposure_min = info.exposure_min;
  SensorInfo->exposure_max = info.exposure_max * CAM_LINE_SCALE;	//lines are CAM_LINE_SCALE x longer

  return ISP_OK;
}

/**
  * @brief  ISP Middleware helper. Camera gain setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
  UNUSED(Instance);
  isp_gain = Gain;
  return (ISP_StatusTypeDef) IMX335_SetGain(&IMX335Obj, Gain);
}

/**
  * @brief  ISP Middleware helper. Camera gain getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain)
{
  UNUSED(Instance);
  *Gain = isp_gain;
  return ISP_OK;
}

/**
  * @brief  ISP Middleware helper. Camera exposure setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure)
{
  UNUSED(Instance);
  isp_exposure = Exposure;
  /* The driver converts us to lines assuming 7.4us lines; ours are CAM_LINE_SCALE x longer */
  return (ISP_StatusTypeDef) IMX335_SetExposure(&IMX335Obj, Exposure / CAM_LINE_SCALE);
}

/**
  * @brief  ISP Middleware helper. Camera exposure getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
  UNUSED(Instance);
  *Exposure = isp_exposure;
  return ISP_OK;
}

void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  NbMainFrames++;
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  /* Update the frame counter and call the ISP statistics handler */
  switch (Pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&hcamera_isp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&hcamera_isp);
      ISP_GatherStatistics(&hcamera_isp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&hcamera_isp);
      break;
  }
}
/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval error status
  */
static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat)
{
  IMX335_IO_t              IOCtx;
  uint32_t                 id;

  /* Configure the camera driver */
  IOCtx.Address     = CAMERA_IMX335_ADDRESS;
  IOCtx.Init        = BSP_I2C1_Init;
  IOCtx.DeInit      = BSP_I2C1_DeInit;
  IOCtx.ReadReg     = BSP_I2C1_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C1_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if (IMX335_RegisterBusIO(&IMX335Obj, &IOCtx) != IMX335_OK)
  {
    Error_Handler();
  }
  else if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK)
  {
    Error_Handler();
  }
  else
  {
    if (id != (uint32_t) IMX335_CHIP_ID)
    {
      Error_Handler();
    }
    else
    {
      if (IMX335_Init(&IMX335Obj, Resolution, PixelFormat) != IMX335_OK)
      {
        Error_Handler();
      }
      else if(IMX335_SetFrequency(&IMX335Obj, IMX335_INCK_24MHZ)!= IMX335_OK)	//6, 18, 24
      {
        Error_Handler();
      }
      else
      {
#if (CAM_LINE_SCALE > 1)
        /* Stretch each sensor line (HMAX, 16 bit at 0x3034) so the data rate fits PSRAM */
        uint16_t hmax = 0;
        uint8_t hold;
        if (imx335_read_reg(&IMX335Obj.Ctx, IMX335_REG_HMAX, (uint8_t *)&hmax, 2) != IMX335_OK)
        {
          Error_Handler();
        }
        HAL_UART_Transmit(&huart1, (unsigned char*)("CAM HMAX "), 9, 100);
        UARTshort(hmax, 4);
        hmax *= CAM_LINE_SCALE;
        HAL_UART_Transmit(&huart1, (unsigned char*)(" -> "), 4, 100);
        UARTshort(hmax, 4);
        UARTreturn();
        hold = 1;
        if (imx335_write_reg(&IMX335Obj.Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK ||
            imx335_write_reg(&IMX335Obj.Ctx, IMX335_REG_HMAX, (uint8_t *)&hmax, 2) != IMX335_OK)
        {
          Error_Handler();
        }
        hold = 0;
        if (imx335_write_reg(&IMX335Obj.Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK)
        {
          Error_Handler();
        }
#endif
#ifdef CAM_TEST_PATTERN
        if (IMX335_SetTestPattern(&IMX335Obj, CAM_TEST_PATTERN) != IMX335_OK)
        {
          Error_Handler();
        }
        HAL_UART_Transmit(&huart1, (unsigned char*)("CAM TEST PATTERN ON\r\n"), 21, 100);
#endif
        return;
      }
    }
  }
}
/***********************************************************************/
/*  CRC   tag IDs took 550usec at intern ref clk,0div
***********************************************************************/
unsigned short calcCRC(unsigned char cbuff[], unsigned int LEN) {
   int i,j;
   unsigned short X = 0xFFFF;
   unsigned short Y = 0x0080;
   unsigned short Z;
   for (i=0;i<LEN;i++){       //for each element
     Y = 0x0080;
     for (j=0;j<8;j++){
       Z = X;
       X <<= 1;
       if((Y & cbuff[i]) != 0){ X++;};
       Y >>= 1;
       if ((Z & 0x8000) != 0) {X ^= 0x1021; };
    };   //end 8x
   };    // end for each element
  for (i=0;i<16;i++){
    if ((X & 0x8000) != 0) { X<<=1; X ^= 0x1021; } else X <<= 1;
  };     //end 16x
  return X;
}

/*********************************************************
 * SENSOR SDA OUTPUT LOW
 *********************************************************/
void sensorSDA_OUTlow(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SDA_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(SENSOR_SDA_GPIO_Port, SENSOR_SDA_Pin, 0);//B11=SensorSDA
}
/*********************************************************
 * SENSOR SCL OUTPUT
 *********************************************************/
void sensorSCL_OUT(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SCL_GPIO_Port, &GPIO_InitStruct);
}
/*********************************************************
 * SENSOR SCL OUTPUT
 *********************************************************/
void sensorSCL_OUTlow(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SCL_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(SENSOR_SCL_GPIO_Port, SENSOR_SCL_Pin, 0);//B10=sensorSCL
}
/*********************************************************
*   SENSOR SDA INPUT PULLUP
*********************************************************/
void sensorSDA_INpull(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SDA_GPIO_Port, &GPIO_InitStruct);
//   while((HAL_GPIO_ReadPin(SENSOR_SDA_PORT, SENSOR_SDA_PIN))==0){}//Wait for SDA to go high.
}
/*********************************************************
*   SENSOR SCL INPUT PULLUP
*********************************************************/
void sensorSCL_INpull(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SCL_GPIO_Port, &GPIO_InitStruct);
  while((HAL_GPIO_ReadPin(SENSOR_SCL_GPIO_Port, SENSOR_SCL_Pin))==0){}//Wait for SCL to go high.
}
/*********************************************************
*   WRITE I2C DATA
*********************************************************/
uint8_t write_data_L(uint8_t* indata, uint8_t numBytes, uint8_t slaveAddress, uint8_t stretch)
{
	uint8_t index, ack = 0;

	twi_start_cond_L();
	if(!send_slave_Address_L(0, slaveAddress))
		return 0;

	for(index = 0; index < numBytes; index++)
	{
		if(index==(numBytes-1))
			ack = i2c_write_byte_L(indata[index], stretch);
		else ack = i2c_write_byte_L(indata[index], 0);
		if(!ack) break;
	}
	//STOP
	if(stretch==0)sensorSCL_INpull();
   myDelay(EE_I2C_DELAY);
// 	HAL_Delay(1); //delay_us(SCL_SDA_DELAY_L);  was one usec in Atmel
	sensorSDA_INpull();
	return ack;

}
/*********************************************************
*   WRITE BYTE SENSORS
*********************************************************/
uint8_t i2c_write_byte_L(uint8_t byte, uint8_t stretch)
{
	uint8_t bit;
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = SENSOR_SCL_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SENSOR_SCL_GPIO_Port, &GPIO_InitStruct);

	for (bit = 0; bit < 8; bit++)
		{
            write_sda_L((byte & 0x80) != 0);
            myDelay(EE_I2C_DELAY);
            sensorSCL_INpull();
            myDelay(EE_I2C_DELAY);
            sensorSCL_OUTlow();
            byte <<= 1;
            myDelay(EE_I2C_DELAY);
		}
	sensorSDA_INpull();
	sensorSCL_INpull(); //goes high for the 9th clock
   myDelay(EE_I2C_DELAY);
   myDelay(EE_I2C_DELAY);
//    if ((stretch==0) && (HAL_GPIO_ReadPin(SENSOR_SDA_PORT, SENSOR_SDA_PIN)==1)){
//     	twi_stop_cond_L();
//     	return 0;
//    }
   myDelay(EE_I2C_DELAY);
   sensorSCL_OUTlow();
   sensorSDA_OUTlow();
//    if(stretch){
//        HAL_Delay(1);
//    }
//    if (stretch && (HAL_GPIO_ReadPin(SENSOR_SDA_PORT, SENSOR_SDA_PIN)==1)){
//     	twi_stop_cond_L();
//    }
   return 1;
}
/*********************************************************
*   READ DATA SENSORS
*********************************************************/
uint8_t read_bytes_L(uint8_t* data, uint8_t bytes, uint8_t slaveAddress)
{
	uint8_t index,success = 0;
	twi_start_cond_L();
	send_slave_Address_L(1, slaveAddress);
	for(index = 0; index < bytes; index++)
	{
		success = i2c_read_byte_L(data, bytes, index);//db: always returns 1?
// 		if(!success)
// 			break;
	}
	twi_stop_cond_L();
	return success;
}
/*********************************************************
*   READ BYTE SENSORS
*********************************************************/
uint8_t i2c_read_byte_L(uint8_t* rcvdata, uint8_t bytes, uint8_t index)
{
	uint8_t byte = 0;
	uint8_t bit = 0;
	sensorSDA_INpull();
	for (bit = 0; bit < 8; bit++)
	{
		sensorSCL_INpull();
		myDelay(EE_I2C_DELAY);
		if(HAL_GPIO_ReadPin(SENSOR_SDA_GPIO_Port, SENSOR_SDA_Pin))
				  byte|= (1 << (7- bit));
		sensorSCL_OUTlow();
		myDelay(EE_I2C_DELAY);
        }
	rcvdata[index] = byte;
	if(index < (bytes-1))//if not the last byte, ACK the slave
	{
		sensorSDA_OUTlow();	//ACK
		sensorSCL_INpull(); //SCL high for 9th clock
	    myDelay(EE_I2C_DELAY);
		sensorSCL_OUTlow(); //SCL low after 9th clock
	    myDelay(EE_I2C_DELAY);
		sensorSDA_INpull();//release SDA after ACK
	}
	else //send NACK on the last byte
	{
		sensorSDA_INpull(); //NACK
		sensorSCL_INpull(); //SCL high for the 9th clock
	    myDelay(EE_I2C_DELAY);
		sensorSCL_OUTlow(); //SCL low after 9th clock
	    myDelay(EE_I2C_DELAY);
		sensorSCL_INpull(); //goes high for the 9th clock
	    myDelay(EE_I2C_DELAY);
		while(!HAL_GPIO_ReadPin(SENSOR_SCL_GPIO_Port, SENSOR_SCL_Pin)){}
	}
	return 1;
}
/*********************************************************
*  WRITE SDA PIN
*********************************************************/
void write_sda_L (uint8_t x)
{
	if(x) sensorSDA_INpull();
	else sensorSDA_OUTlow();
}
/*********************************************************
*   SEND SLAVE ADDRESS
*********************************************************/
uint8_t send_slave_Address_L(uint8_t read, uint8_t slaveAddress)
{
 	return i2c_write_byte_L(slaveAddress | read, 0 );	//write_byte returns 1, so this returns 0.
}
/*********************************************************
*   START CONDITION
*********************************************************/
uint8_t twi_start_cond_L(void)
{
	sensorSCL_INpull();
	sensorSDA_OUTlow();
   myDelay(EE_I2C_DELAY);
	sensorSDA_OUTlow();
   myDelay(EE_I2C_DELAY);
	return 1;
}
/*********************************************************
*   STOP CONDITION
*********************************************************/
uint8_t twi_stop_cond_L(void)
{
	sensorSDA_OUT();
   myDelay(EE_I2C_DELAY);
	sensorSCL_INpull();
   myDelay(EE_I2C_DELAY);
	return 1;
}
/*********************************************************
 * SENSOR SDA OUTPUT
 *********************************************************/
void sensorSDA_OUT(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = SENSOR_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SENSOR_SDA_GPIO_Port, &GPIO_InitStruct);
}
/*********************************************************
 * SENSOR ALS FROM TINY APDS
 *********************************************************/
uint16_t getALSfromTinyAPDS(uint8_t slaveAddress){
 	uint8_t data[4];
 	uint8_t iters;
 	uint16_t lightMeasurement;
 	sensorSCL_INpull();
 	sensorSDA_INpull();
 	HAL_Delay(10);
 	data[0]=0x00;//
 	data[1]=0x02;//Enable light conversion
 	write_data_L(data,2,slaveAddress,0);
 	sensorSCL_INpull();
 	sensorSDA_INpull();
 	HAL_Delay(50);
 	data[0]=0x0D;//start address for read
 	write_data_L(data,1,slaveAddress,0);
 	sensorSCL_INpull();
 	sensorSDA_INpull();
 	for(iters=0; iters<3; iters++){	//clear the buffer
 		data[iters]=0;
 	}
 	read_bytes_L(data,2,slaveAddress);
 	sensorSCL_INpull();
 	sensorSDA_INpull();
 	lightMeasurement = (data[1] << 3) + (data[0]>>5);
 	if((data[2]>0)||(data[1]>0x1F)){
 		lightMeasurement = 0xFF;
 	}
 	return(lightMeasurement);
}
/*********************************************************/
/* MY FAST DELAY                                         */
/*********************************************************/
void myDelay(uint32_t myTicks){
	int iters,itersInner;
	for (iters=myTicks; iters>0; iters--){
		for (itersInner=TICKCTR; itersInner>0; itersInner--){}
	}
}

#ifdef SEND_3GRAYS
/*********************************************************/
/* CREATE GRAYSCALE FROM COLOR                          */
/*********************************************************/
void createGrayscale(uint8_t whichGrayscale){
	uint8_t* destBufferPtr = Main_DestBuffer;
	uint8_t* grayBufferPtr;
	uint8_t grayVal;
	uint32_t iters;

	if(whichGrayscale==0) grayBufferPtr=grayscale_A;
	if(whichGrayscale==1) grayBufferPtr=grayscale_B;

	for(iters=0;iters<921600;iters+=3){
		grayVal=(*destBufferPtr/3) + (*(destBufferPtr+1)/3) + (*(destBufferPtr+2)/3);
		*grayBufferPtr++=grayVal;
		destBufferPtr+=3;
	}
}
/**********************************************************************************************************************************/
/* MERGE GRAYSCALE BUFFER (so we can send one big packet with three grayscale images instead of one color image.                  */
/* also converts the current color image in the color buffer into a grayscale, then it packs the previous two grayscales after it */
/**********************************************************************************************************************************/
void mergeGrayscales(void){
	uint8_t* mainColorBufferPtr = Main_DestBuffer;
	uint8_t* mainGrayBufferPtr = Main_DestBuffer;
	uint8_t* grayBuffer_A_Ptr = grayscale_A;
	uint8_t* grayBuffer_B_Ptr = grayscale_B;
	uint8_t grayVal;
	uint32_t iters;

	for(iters=0;iters<921600;iters+=3){
		grayVal=(*mainColorBufferPtr/3) + (*(mainColorBufferPtr+1)/3) + (*(mainColorBufferPtr+2)/3);
		*mainGrayBufferPtr++ = grayVal;
		mainColorBufferPtr += 3;
	}

	for(iters=0;iters<307200;iters++){
		*mainGrayBufferPtr++ = *grayBuffer_A_Ptr++;
	}
	for(iters=0;iters<307200;iters++){
		*mainGrayBufferPtr++ = *grayBuffer_B_Ptr++;
	}

}
#endif	//grayscales
/*********************************************************/
/* TAKE PIC                                              */
/*********************************************************/
void takePic(void){
	uint8_t res;
	  ISP_AppliHelpersTypeDef appliHelpers;

	  /* Fill init struct with Camera driver helpers */
	  appliHelpers.GetSensorInfo = GetSensorInfoHelper;			//from Snapshot
	  appliHelpers.SetSensorGain = SetSensorGainHelper;			//from Snapshot
	  appliHelpers.GetSensorGain = GetSensorGainHelper;			//from Snapshot
	  appliHelpers.SetSensorExposure = SetSensorExposureHelper;			//from Snapshot
	  appliHelpers.GetSensorExposure = GetSensorExposureHelper;			//from Snapshot
	  /* Initialize the Image Signal Processing middleware */
//	  res = ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, &ISP_IQParamCacheInit_IMX335);            // options for param configs in core/inc/imx335_E27_isp_param_conf.h
//	  if(res != ISP_OK){
//			  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP INIT FAIL\r\n"), 15, 100);
//			  Error_Handler();
//		  }else{
//			  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP INIT GOOD\r\n"), 15, 100);
//		  }

	  captureCheckPrepare();	//capture debug: fill buffer with 0x5A, zero the counters
	  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0 , (uint8_t *) Main_DestBuffer, DCMIPP_MODE_CONTINUOUS) != HAL_OK)	//DCMIPP_MODE_SNAPSHOT  //DCMIPP_MODE_CONTINUOUS
	  {
		  HAL_UART_Transmit(&huart1, (unsigned char*)("PIPE START FAIL\r\n"), 17, 100);
		  Error_Handler();
	  }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("PIPE START GOOD\r\n"), 17, 100);
	  }
	  /* Start the Image Signal Processing */
	  if (ISP_Start(&hcamera_isp) != ISP_OK)
	  {
		  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP START FAIL\r\n"), 16, 100);
		  Error_Handler();
	  }else{
		  HAL_UART_Transmit(&huart1, (unsigned char*)("ISP START GOOD\r\n"), 16, 100);
	  }
	  NbMainFrames=0;
	  while(NbMainFrames < 30)	//  30/SEC. WAS 60 BEFORE MARCH 8. 5=BLACK. 15=TOO WHITE. 25=A LITTLE DARK. // options for param configs in core/inc/imx335_E27_isp_param_conf.h
	  {
		  ispBackground();	//auto exposure and auto white balance on each new frame's statistics
	  }
	  /* stop the acquisition */
	  HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0);
	  HAL_UART_Transmit(&huart1, (unsigned char*)("DONE FRAMES\r\n"), 13, 100);
	  captureCheckReport();	//capture debug: overruns, ISP errors, 0xFF / unwritten runs

}

/*********************************************************/
/* ISP BACKGROUND PROCESS + CAPTURE CHECKS (debug)       */
/*********************************************************/
static volatile uint32_t p1OverrunCount = 0;
static uint32_t ispErrCount = 0;
static uint32_t ispFirstErr = 0;

/* Runs the ISP algorithms (AEC, AWB). Errors are counted, and the first error code kept,
 * instead of printed every time, so the loop isn't slowed by the UART. */
void ispBackground(void){
	ISP_StatusTypeDef status = ISP_BackgroundProcess(&hcamera_isp);
	if(status != ISP_OK){
		if(ispErrCount == 0) ispFirstErr = status;
		ispErrCount++;
	}
}

/* Pipe1 overrun: count every one. The HAL turns the overrun interrupt off after each,
 * so turn it back on. */
void HAL_DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
	if(Pipe == DCMIPP_PIPE1){
		p1OverrunCount++;
		__HAL_DCMIPP_ENABLE_IT(hdcmipp, DCMIPP_IT_PIPE1_OVR);
	}
}

#define CHECK_FILL 0x5A5A5A5AUL	//anything still 0x5A after capture was never written by DCMIPP
#ifdef RESOLUTION_2400_1600
#define CHECK_PITCH (IMAGE_WIDTH * 3)	//bytes per image row
#else
#define CHECK_PITCH 1920
#endif

void captureCheckPrepare(void){
	volatile uint32_t *words = (volatile uint32_t *) Main_DestBuffer;
	uint32_t iters;
	for(iters=0; iters<sizeof(Main_DestBuffer)/4; iters++){
		words[iters] = CHECK_FILL;
	}
	p1OverrunCount = 0;
	ispErrCount = 0;
	ispFirstErr = 0;
}

/* Prints the total number of words equal to 'value' and the first runs of at least 64 bytes,
 * as image row / pixel column / length in bytes */
static void reportRuns(uint32_t value, const char *name, uint8_t nameLen){
	volatile uint32_t *words = (volatile uint32_t *) Main_DestBuffer;
	uint32_t numWords = sizeof(Main_DestBuffer)/4;
	uint32_t iters, total = 0, runStart = 0, runLen = 0, printed = 0;
	uint32_t pitch = CHECK_PITCH;

	for(iters=0; iters<=numWords; iters++){
		if(iters < numWords && words[iters] == value){
			if(runLen == 0) runStart = iters;
			runLen++;
			total++;
		}else if(runLen > 0){
			if(runLen >= 16 && printed < 20){
				uint32_t offset = runStart * 4;
				HAL_UART_Transmit(&huart1, (unsigned char*)name, nameLen, 100);
				HAL_UART_Transmit(&huart1, (unsigned char*)(" RUN ROW "), 9, 100);
				UARTint(offset / pitch);
				HAL_UART_Transmit(&huart1, (unsigned char*)(" COL "), 5, 100);
				UARTint((offset % pitch) / 3);
				HAL_UART_Transmit(&huart1, (unsigned char*)(" BYTES "), 7, 100);
				UARTint(runLen * 4);
				UARTreturn();
				printed++;
			}
			runLen = 0;
		}
	}
	HAL_UART_Transmit(&huart1, (unsigned char*)name, nameLen, 100);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" TOTAL BYTES "), 13, 100);
	UARTint(total * 4);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" OF "), 4, 100);
	UARTint(sizeof(Main_DestBuffer));
	UARTreturn();
}

/* Prints where the colour changes sharply between neighbouring pixels (first 12 places), as
 * row / column, with the RGB before and after. With the sensor's horizontal colour bars
 * (CAM_TEST_PATTERN 10) every edge should be at column 0; edges in mid-row mean the image
 * is offset within the camera's buffer. */
static void reportEdges(void){
	volatile uint8_t *buf = (volatile uint8_t *) Main_DestBuffer;
	uint32_t numPixels = sizeof(Main_DestBuffer) / 3;
	uint32_t pix, found = 0;
	int32_t diff, maxDiff;
	uint8_t ch;

	for(pix=1; pix<numPixels && found<12; pix++){
		maxDiff = 0;
		for(ch=0; ch<3; ch++){
			diff = (int32_t) buf[pix*3 + ch] - (int32_t) buf[(pix-1)*3 + ch];
			if(diff < 0) diff = -diff;
			if(diff > maxDiff) maxDiff = diff;
		}
		if(maxDiff > 64){
			HAL_UART_Transmit(&huart1, (unsigned char*)("EDGE ROW "), 9, 100);
			UARTint(pix / (CHECK_PITCH / 3));
			HAL_UART_Transmit(&huart1, (unsigned char*)(" COL "), 5, 100);
			UARTint(pix % (CHECK_PITCH / 3));
			HAL_UART_Transmit(&huart1, (unsigned char*)(" RGB "), 5, 100);
			for(ch=0; ch<3; ch++) UARTshort(buf[(pix-1)*3 + ch], 2);
			HAL_UART_Transmit(&huart1, (unsigned char*)(" -> "), 4, 100);
			for(ch=0; ch<3; ch++) UARTshort(buf[pix*3 + ch], 2);
			UARTreturn();
			found++;
		}
	}
}

void captureCheckReport(void){
	HAL_UART_Transmit(&huart1, (unsigned char*)("P1 OVERRUNS "), 12, 100);
	UARTint(p1OverrunCount);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" ISP ERRS "), 10, 100);
	UARTint(ispErrCount);
	HAL_UART_Transmit(&huart1, (unsigned char*)(" FIRST "), 7, 100);
	UARTint(ispFirstErr);
	UARTreturn();
	reportRuns(CHECK_FILL, "UNWRITTEN", 9);
	reportRuns(0xFFFFFFFFUL, "FF", 2);
	reportEdges();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  HAL_UART_Transmit(&huart1, (unsigned char*)("ERROR HANDLER\r\n"), 15, 100);
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
