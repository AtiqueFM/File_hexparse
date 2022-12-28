

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "main.h"

#define SET                 1
#define RESET               !SET
#define PRINT_HEX_DATA      SET
//#define PRINT_DATA

/*
    HEX file frame:- |1byte (Length)|2bytes (offset address)| 1bytes (type of data)|nbytes (data)| 1byte (data check)| 
*/
enum{
    data_length = 0,
    offset_address,
    data_type = 3,
    data_start,
}HEXFILE_FRAME_FORMAT;

enum{
    DATA_RECORD,                        //0
    END_OF_FILE,                        //1
    EXTENED_SEGMENT_ADDRESS_RECORD,     //2
    START_SEGMENT_ADDRESS_RECORD,       //3
    EXTENDED_SEGMENT_ADDRESS_RECORD,    //4
    START_LINEAR_ADDRESS_RECORD,        //5
}HEXFILE_RECORD_TYPE;

enum{
    BIT_24 = 24,
    BIT_16 = 16,
    BIT_8 = 8,
}NO_OF_BITS;

/*Buffer for reading the hex file data char format*/
char hex_file_code[10000];
/*Converting the character datatype into hex*/
int hex_pair[10000];
/*Varibale to store the Flash address starting address*/
int flash_addr;
/*Reset handler address*/
int reset_handler_addr;
/*Hex file path*/
char filename[] = "/home/atiqueshaikh/Forbes Marshall - Projects/STM32_UART/awa-cxii_1/Debug/awa-cxii_1.hex";
/*Hex file parsed*/
char filename_MCU[] = "Build/Read_data.txt";
/*Info file*/
char filename_info[] = "Build/Info.txt";
/*Info file*/
char filename_config_data[] = "Build/Config_data.txt";
/*Final merged File*/
char filename_final_merge[] = "Build/program.txt";
/*Row hex data*/
int row_data[50];
/*length of row data*/
unsigned int length;
/*32 bits to 8 bits*/
typedef union{
    uint8_t bytes[4];
    struct{
        uint32_t u32data;
    };
}u32tou8Handle_t;
/*Flag to store the read data in the file*/
uint8_t store_in_file = RESET;
/*Number of rows*/
uint32_t no_of_rows;
/*Configuration data structure*/
bootloader_handle_t struct_BOOTLODERDATA;

/*
Function name :- reverse
return type :- u32tou8Handle_t
arguments :- u32tou8Handle_t
Description :- 
                array reverse
*/
u32tou8Handle_t reverse(u32tou8Handle_t arg)
{
    u32tou8Handle_t temp;
    temp.bytes[3] = arg.bytes[0];
    temp.bytes[2] = arg.bytes[1];
    temp.bytes[1] = arg.bytes[2];
    temp.bytes[0] = arg.bytes[3];

    return temp;
}

/*
Fucntion prototyping
*/
static void add_bootloader_config_data(FILE *ptr_txt_file);
static void config_data_in_file(FILE *ptr_txt_file,uint8_t *pAddress,uint8_t *pData);
static void fill_data(u32tou8Handle_t *l_flashaddrTobytes,u32tou8Handle_t *input_data_segg,uint32_t address, uint32_t data);
static void set_default_config_data(void);
uint16_t crc_calc(char* input_str, int len );
/*
Function name :- getHEXfileData
return type :- int
arguments :- None
Description :- 
                Seggrigates the file into flash address and its data.
                Stores the hex data into an new integer array.
Gloabl Varibale :- 
                    hex_pair
                            type :- integer array
                            length :- 100
                            Description :- contains the final hex data except the flash address.
                    flash_addr
                            type :- integer
                            length :- 1
                            Description :- stores the starting address of FLASH memory
Local Varibale :-
                    data_len
                            type :- char
                            Length :- 1 
                            Description :- stores the data in a single row if the file
                    
                    offset_addr
                            type :- unsigned int
                            Length :- 1 
                            Description :- stores the offset flash address in a single row if the file    
                    data_len
                            type :- char
                            Length :- 1 
                            Description :- stores the bytes of data in a single row if the file
                    data_typ
                            type :- char
                            Length :- 1 
                            Description :- stores the type of data in a single row if the file
*/
int getHEXfileData()
{
    //Data length in the row
    char data_len = hex_pair[data_length];
    //Offset address for base flash address STARTING_FLASH_ADDRESS + offset_address <- Data bytes
    unsigned int offset_addr = (hex_pair[offset_address]<<8) | hex_pair[offset_address+1];
    //printf("offset address: %x\n",offset_addr);
    char data_typ = hex_pair[data_type];
    
    int j = 0;

    u32tou8Handle_t flashaddrTobytes;

    //printf("Data type: %x\n",data_typ);
    //printf("CRC: %x\n",hex_pair[data_start+data_len]);
    
    switch(data_typ)
    {
        case DATA_RECORD:
        /*
        Actual HEX data to be sent over UART/SPI to target MCU.
        */
        #if PRINT_HEX_DATA
            #if defined(PRINT_DATA)
            printf("%x | ",flash_addr + offset_addr);
            #endif
            flashaddrTobytes.u32data = flash_addr + offset_addr;
            flashaddrTobytes = reverse(flashaddrTobytes);
            for(int i = 0;i <= (data_len + 5);i++,j++)
            {
                if(i < 4)
                {
                    row_data[j] = flashaddrTobytes.bytes[i];
                }
                else if(i == 4)
                {
                    row_data[j] = data_len;
                }else if(i > 4)
                {
                #if defined(PRINT_DATA)
                //Send over uart with FLASH address first
                printf("%x ",hex_pair[data_start + i]);
                #endif
                row_data[j] = hex_pair[data_start + i - 5];
                //j += 1;
                }
            }
            length = data_len + 5 - 1;// 4 bytes for 32 bit FLASH address + 1 byte for number of data bytes
            no_of_rows += 1;
            store_in_file = SET;
            #if defined(PRINT_DATA)
            printf("\n");
            #endif
        #endif
        break;
        case END_OF_FILE:
            //Its the end of HEX FILE
            printf("End of HEX FILE\n");
        break;
        
        case EXTENDED_SEGMENT_ADDRESS_RECORD:
        /*
        Starting FLASH address
        */
            flash_addr = (hex_pair[data_start] << BIT_24)|(hex_pair[data_start + 1] << BIT_16);
            printf("flash address: %x\n",flash_addr);
            //store_in_file = SET;
        break;
        
        case START_LINEAR_ADDRESS_RECORD:
        /*
        RESET handler address (Already stored in the VECTOR TABLE)
        */
            reset_handler_addr = (hex_pair[data_start] << BIT_24)|(hex_pair[data_start + 1] << BIT_16)
                                    |(hex_pair[data_start + 2] << BIT_8)|(hex_pair[data_start + 3]);
            printf("Reset handler address: %x\n",reset_handler_addr);
        break;
        
        default:
            printf("Unknown record type: %x\n",data_typ);
        break;
        
    }

}

void workaround(FILE *ptr)
{
    #if 0
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x ", 0x5a5);
    #else
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a);
    #endif
}

// Driver code
int main()
{
	FILE* ptr;
	FILE* ptr_txt_file;
	char ch;
	
	int index = 0;

	// Opening file in reading mode
	ptr = fopen(filename, "r");

	if (NULL == ptr) {
		printf("file can't be opened \n");
	}

	// Opening file to write the parsed data
	ptr_txt_file = fopen(filename_MCU, "wb+");
    workaround(ptr_txt_file);
    if (NULL == ptr_txt_file) {
		printf("file can't be opened \n");
	}
	printf("content of this file are \n");

    add_bootloader_config_data(ptr_txt_file);

    
#if 0
    //---------------------------------------------//

	do {
		ch = fgetc(ptr);
		
		if(ch == ':')
		    continue;
// 		printf("%c", ch);
        hex_file_code[index] = (int)strtol(&ch, NULL, 16);
        
        //printf("%X ",hex_file_code[index]);
        
        index += 1;
        if(ch == '\n')
        {
            //printf("\n");
            int i = 0;
            int j = 0;
            while(i < (index-1)/2)
            {
                hex_pair[i] = (hex_file_code[j] << 4) | (hex_file_code[++j]);
                //fclose(ptr_txt_file);+= 1;
                

            }
            getHEXfileData();
            if(store_in_file == SET)
            {
                store_in_file = RESET;
                //Write the roe hex data into new file
                char d = ' ';
                fprintf(ptr_txt_file, "%c",d);//Writing space before the starting byte
                int k = 0;
                for(k = 0;k<=length;k++)
                    fprintf(ptr_txt_file, "%x ", row_data[k]);//writing the actual data
                fprintf(ptr_txt_file,"\n");
                memset(row_data,'\0',sizeof(row_data));
                length = 0;
            }
            index = 0;//test
        }
        // Checking if character is not EOF.
        // If it is EOF stop eading.
    	} while (ch != EOF);
	// Closing the file
	fclose(ptr);

#endif
    //get the size of the hex data
    long int res = ftell(ptr_txt_file);

	// Opening file to write the parsed data
    fseek(ptr_txt_file,0,SEEK_SET);

    //Storning the number of rows
    uint8_t temp[4];
    u32tou8Handle_t stru;
    stru.u32data = no_of_rows + sizeof(struct_BOOTLODERDATA.u32array)/4;//5 additional confguration data
    stru = reverse(stru);
    char d = ' ';
    fprintf(ptr_txt_file, "%c",d);
    for(int k = 0;k<4;k++)
        fprintf(ptr_txt_file, "%x ", stru.bytes[k]);
    fprintf(ptr_txt_file,"\n");

    //storing the file size
    stru.u32data = res;
    stru = reverse(stru);
    d = ' ';
    fprintf(ptr_txt_file, "%c",d);
    for(int k = 0;k<4;k++)
        fprintf(ptr_txt_file, "%x ", stru.bytes[k]);
    fprintf(ptr_txt_file,"\n");
    fclose(ptr_txt_file);
	return 0;
}


static void add_bootloader_config_data(FILE *ptr_txt_file)
{

    uint32_t input_data = 0;
    uint32_t l_data_length = 4;
    int j = 0;
    char l_row_data[10] = {0};
    u32tou8Handle_t l_flashaddrTobytes;
    u32tou8Handle_t input_data_segg;

#if 0
    fill_data(&l_flashaddrTobytes,&input_data_segg,0x8010000,0x12345678);
    config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);

    fill_data(&l_flashaddrTobytes,&input_data_segg,0x8010004,0x11223344);
    config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);

    fill_data(&l_flashaddrTobytes,&input_data_segg,0x8010008,0xaabbccdd);
    config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);

    fill_data(&l_flashaddrTobytes,&input_data_segg,0x801000c,0x88551122);
    config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);

    fill_data(&l_flashaddrTobytes,&input_data_segg,0x8010010,0x44663322);
    config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);
#else
    set_default_config_data();
    static uint32_t address = 0x8010000;
    uint32_t *pData = NULL;
    pData = (uint32_t*)&struct_BOOTLODERDATA.u32array[0];
    for(int i = 0;i<sizeof(bootloader_handle_t)/4;i+=1,address += 4)
    {
        fill_data(&l_flashaddrTobytes,&input_data_segg, address,*pData);
        config_data_in_file(ptr_txt_file,l_flashaddrTobytes.bytes,input_data_segg.bytes);
        pData += 1;
    }
#endif

}

static void config_data_in_file(FILE *ptr_txt_file,uint8_t *pAddress,uint8_t *pData)
{
    uint8_t l_row_data[10] = {0};
    int l_data_length = 4;
    int j = 0;

    for(int i = 0;i <= (l_data_length + 5);i++,j++)
    {
        if(i < 4)
        {
            l_row_data[j] = pAddress[i];
        }
        else if(i == 4)
        {
            l_row_data[j] = l_data_length;
        }else if(i > 4)
        {
        l_row_data[j] = pData[i - 5];
        }
    }
    //print in the file
    char t = ' ';
    fprintf(ptr_txt_file, "%c",t);//Writing space before the starting byte
    for(int i = 0;i<9;i++)
        fprintf(ptr_txt_file, "%x ", l_row_data[i]);//writing the actual data
    fprintf(ptr_txt_file,"\n");
}

static void fill_data(u32tou8Handle_t *l_flashaddrTobytes,u32tou8Handle_t *input_data_segg,uint32_t address, uint32_t data)
{
    input_data_segg->u32data = data;
    l_flashaddrTobytes->u32data = address;
    *l_flashaddrTobytes = reverse(*l_flashaddrTobytes);
    //*input_data_segg = reverse(*input_data_segg);
}

static void set_default_config_data(void)
{
    uint32_t lu32_crc = 0;
    memset(struct_BOOTLODERDATA.u8array,0,sizeof(struct_BOOTLODERDATA.u8array));

    //BOOTLOADER
    strcpy(struct_BOOTLODERDATA.reboot_string,STARTUP_STRING);
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.BOOT_SEQUENCE = PROGRAM_SEQ_A;
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.BOOTLOADER_PARTITION_SIZE = BOOTLOADER_PARTITION_SIZE_;
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.PARTION_A_RESET_HANDLER_ADDRESS = PARTITION_A_RESET_HANDLER;
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.PARTION_A_FLASH_SIZE = PARTITION_A_FLASH_SIZE;
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.PARTION_B_RESET_HANDLER_ADDRESS = PARTITION_B_RESET_HANDLER;
    struct_BOOTLODERDATA.BOOTLOADER_CONFIG_DATA.PARTION_B_FLASH_SIZE = PARTITION_B_FLASH_SIZE;

    //ACTIVE PROGRAM
    struct_BOOTLODERDATA.ACTIVE_PROGRAM.program_ResetHandler_address = ACTIVE_PROGRAM_RESETHANDLER;
    struct_BOOTLODERDATA.ACTIVE_PROGRAM.program_size = ACTIVE_PROGRAM_SIZE;
    //strcpy((char*)&struct_BOOTLODERDATA.ACTIVE_PROGRAM.program_partion,ACTIVE_PROGRAM_PARTITION);
    //strcpy((char*)&struct_BOOTLODERDATA.ACTIVE_PROGRAM.program_Author,ACTIVE_PROGRAM_AUTHOR);
    //strcpy((char*)&struct_BOOTLODERDATA.ACTIVE_PROGRAM.program_Firmware_version,ACTIVE_PROGRAM_FW_VERSION);

    //BACKUP PROGRAM
    struct_BOOTLODERDATA.BACKUP_PROGRAM.program_ResetHandler_address = BACKUP_PROGRAM_RESETHANDLER;
    struct_BOOTLODERDATA.BACKUP_PROGRAM.program_size = BACKUP_PROGRAM_SIZE;
    //strcpy((char*)&struct_BOOTLODERDATA.BACKUP_PROGRAM.program_partion,BACKUP_PROGRAM_PARTITION);
    //strcpy((char*)&struct_BOOTLODERDATA.BACKUP_PROGRAM.program_Author,BACKUP_PROGRAM_AUTHOR);
    //strcpy((char*)&struct_BOOTLODERDATA.BACKUP_PROGRAM.program_Firmware_version,BACKUP_PROGRAM_FW_VERSION);

    lu32_crc = crc_calc(struct_BOOTLODERDATA.u8array,sizeof(struct_BOOTLODERDATA.u8array) - 4);
    struct_BOOTLODERDATA.crc_16_bits = lu32_crc;
}

uint16_t crc_calc(char* input_str, int len )
{
    int pos = 0,i = 0;
    uint16_t crc1 = CRC_START_MODBUS;
    for (pos = 0; pos < len; pos++)
    {
        crc1 ^= (uint16_t)input_str[pos];            // XOR byte into least significant byte of CRC
        for (i = 8; i != 0; i--)
        {                                               // Loop over each bit
            if ((crc1 & 0x0001) != 0)
            {                                           // If the LSB is set
                crc1 >>= 1;                              // Shift right and XOR 0xA001
                crc1 ^= CRC_POLY_16;
            }
            else
            {                                           // Else LSB is not set
                crc1 >>= 1;
            }                                           // Just shift right
        }
    }
    return crc1;
}