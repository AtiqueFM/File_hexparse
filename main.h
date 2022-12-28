#include<stdint.h>

#define STARTUP_STRING  "DEADBEEF"
#define CRC_POLY_16         		0xA001
#define CRC_START_MODBUS    		0xFFFF

/*Partition*/

/*Bootloader configuration data*/
#define BOOTLOADER_PARTITION_SIZE_   64
#define PARTITION_A_RESET_HANDLER   0x8020000UL
#define PARTITION_B_RESET_HANDLER   0x80A0000UL
#define PROGRAM_SEQ_A               1UL
#define PROGRAM_SEQ_B               2UL
#define PARTITION_A_FLASH_SIZE      512UL
#define PARTITION_B_FLASH_SIZE      384UL

/*Application configuration data*/
/*Active program*/
#define ACTIVE_PROGRAM_RESETHANDLER PARTITION_A_RESET_HANDLER
#define ACTIVE_PROGRAM_SIZE         PARTITION_A_FLASH_SIZE
#define ACTIVE_PROGRAM_PARTITION    "A"
#define ACTIVE_PROGRAM_AUTHOR       "ATIQUE SHAIKH"
#define ACTIVE_PROGRAM_FW_VERSION   "1.0.0"
/*Backup program*/
#define BACKUP_PROGRAM_RESETHANDLER PARTITION_B_RESET_HANDLER
#define BACKUP_PROGRAM_SIZE         PARTITION_B_FLASH_SIZE
#define BACKUP_PROGRAM_PARTITION    "B"
#define BACKUP_PROGRAM_AUTHOR       "ATIQUE SHAIKH"
#define BACKUP_PROGRAM_FW_VERSION   "1.0.0"

typedef struct __attribute__((packed))
{
	uint32_t BOOTLOADER_PARTITION_SIZE;
	uint32_t BOOT_SEQUENCE;
	uint32_t PARTION_A_RESET_HANDLER_ADDRESS;
	uint32_t PARTION_A_FLASH_SIZE;
	uint32_t PARTION_A_CCRAM_SIZE;
	uint32_t PARTION_B_RESET_HANDLER_ADDRESS;
	uint32_t PARTION_B_FLASH_SIZE;
	uint32_t PARTION_B_CCRAM_SIZE;
}bootloader_configuration_handle_t;

typedef struct __attribute__((packed)){
	uint32_t program_ResetHandler_address;
	uint32_t program_16bit_crc;
	uint32_t program_size;
	uint32_t program_partion;
	uint32_t program_loadtimestamp;
	uint32_t program_reeived_timestamp;
	uint32_t program_Firmware_version;
	uint32_t program_Author;
	uint32_t program_status_flags;
}program_configuration_hanlde_t;


typedef union __attribute__((packed)){

	uint8_t u8array[(sizeof(uint8_t) * 8)
					+ sizeof(bootloader_configuration_handle_t)
					+ sizeof(program_configuration_hanlde_t)
					+ sizeof(program_configuration_hanlde_t)
					+ sizeof(uint32_t)
					+ sizeof(uint32_t)];
	uint32_t u32array[30];

	struct{
		uint8_t reboot_string[8];
		bootloader_configuration_handle_t BOOTLOADER_CONFIG_DATA;
		program_configuration_hanlde_t ACTIVE_PROGRAM;
		program_configuration_hanlde_t BACKUP_PROGRAM;
		uint32_t Failure_state_handle;
		uint32_t crc_16_bits;
	};
}bootloader_handle_t;