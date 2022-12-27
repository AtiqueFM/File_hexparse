

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
char filename[] = "/home/atiqueshaikh/Forbes Marshall - Projects/STM32_UART/awa-cxii/Debug/awa-cxii.hex";
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
static void add_bootloader_config_data(void);

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
                            l.u8arrayescription :- stores the starting address of FLASH memory
Local Varibale :-
                    data_len    //------------------------------------------//
    //-------CONFIGURATION DATA-----------------//
    FILE* ptr_txt_file;
    	// Opening file to write the parsed data
	ptr_txt_file = fopen(filename_config_data, "w");
    if (NULL == ptr_txt_file) {
		printf("file can't be opened \n");
	}

    uint32_t input_data = 0;
    uint32_t l_data_length = 4;
    int j = 0;
    char l_row_data[10] = {0};
    u32tou8Handle_t l_flashaddrTobytes;
    u32tou8Handle_t input_data_segg;

    //printf("BOOTLOADER_PARTION_SIZE: ");
    //scanf("%d",&input_data_segg.u32data);
    input_data_segg.u32data = 0x12345678;
    
    l_flashaddrTobytes.u32data = 0x8010000;
    l_flashaddrTobytes = reverse(l_flashaddrTobytes);
    input_data_segg = reverse(input_data_segg);
    
    for(int i = 0;i <= (l_data_length + 5);i++,j++)
    {
        if(i < 4)
        {
            l_row_data[j] = l_flashaddrTobytes.bytes[i];
        }
        else if(i == 4)
        {
            l_row_data[j] = l_data_length;
        }else if(i > 4)
        {
        l_row_data[j] = input_data_segg.bytes[i - 5];
        }
    }
    //print in the file
    char t = ' ';
    fprintf(ptr_txt_file, "%c",t);//Writing space before the starting byte
    for(int i = 0;i<9;i++)
        fprintf(ptr_txt_file, "%x ", l_row_data[i]);//writing the actual data
    fprintf(ptr_txt_file,"\n"); :- stores the data in a single row if the file
                    
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
            store_in_file = SET;
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
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5a5);
    fprintf(ptr, "%x", 0x5a5);
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

	// Printing what is written in file
	// character by character using loop.

    //add_bootloader_config_data();

    //------------------------------------------//
    //-------CONFIGURATION DATA-----------------//
    //FILE* ptr_txt_file;
    	// Opening file to write the parsed data
	//ptr_txt_file = fopen(filename_config_data, "w");
    //if (NULL == ptr_txt_file) {
		//printf("file can't be opened \n");
	//}

    uint32_t input_data = 0;
    uint32_t l_data_length = 4;
    int j = 0;
    char l_row_data[10] = {0};
    u32tou8Handle_t l_flashaddrTobytes;
    u32tou8Handle_t input_data_segg;

    //printf("BOOTLOADER_PARTION_SIZE: ");
    //scanf("%d",&input_data_segg.u32data);
    input_data_segg.u32data = 0x12345678;
    
    l_flashaddrTobytes.u32data = 0x8010000;
    l_flashaddrTobytes = reverse(l_flashaddrTobytes);
    input_data_segg = reverse(input_data_segg);
    
    for(int i = 0;i <= (l_data_length + 5);i++,j++)
    {
        if(i < 4)
        {
            l_row_data[j] = l_flashaddrTobytes.bytes[i];
        }
        else if(i == 4)
        {
            l_row_data[j] = l_data_length;
        }else if(i > 4)
        {
        l_row_data[j] = input_data_segg.bytes[i - 5];
        }
    }
    //print in the file
    char t = ' ';
    fprintf(ptr_txt_file, "%c",t);//Writing space before the starting byte
    for(int i = 0;i<9;i++)
        fprintf(ptr_txt_file, "%x ", l_row_data[i]);//writing the actual data
    fprintf(ptr_txt_file,"\n");


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
                //printf("%x ",hex_pair[i]);
                j += 1;
                i += 1;
                

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

    //add_bootloader_config_data();
#if 1 //Workaroud
    //get the size of the hex data
    long int res = ftell(ptr_txt_file);

	// Opening file to write the parsed data
    fseek(ptr_txt_file,0,SEEK_SET);

    //Storning the number of rows
    uint8_t temp[4];
    u32tou8Handle_t stru;
    stru.u32data = no_of_rows;
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
#else
	FILE* ptr_info_file;
    	// Opening file to write the parsed data
	ptr_info_file = fopen(filename_info, "wb+");
    if (NULL == ptr_info_file) {
		printf("file can't be opened \n");
	}
        //get the size of the hex data
    long int res = ftell(ptr_txt_file);


    //Storning the number of rows
    uint8_t temp[4];
    u32tou8Handle_t stru;
    stru.u32data = no_of_rows;
    stru = reverse(stru);
    char d = ' ';
    fprintf(ptr_info_file, "%c",d);
    for(int k = 0;k<4;k++)
        fprintf(ptr_info_file, "%x ", stru.bytes[k]);
    fprintf(ptr_info_file,"\n");

    //storing the file size
    stru.u32data = res;
    stru = reverse(stru);
    d = ' ';
    fprintf(ptr_info_file, "%c",d);
    for(int k = 0;k<4;k++)
        fprintf(ptr_info_file, "%x ", stru.bytes[k]);
    fprintf(ptr_info_file,"\n");

    fclose(ptr_info_file);
#endif
	return 0;
}


static void add_bootloader_config_data(void)
{

    //------------------------------------------//
    //-------CONFIGURATION DATA-----------------//
    FILE* ptr_txt_file;
    	// Opening file to write the parsed data
	ptr_txt_file = fopen(filename_config_data, "w");
    if (NULL == ptr_txt_file) {
		printf("file can't be opened \n");
	}

    uint32_t input_data = 0;
    uint32_t l_data_length = 4;
    int j = 0;
    char l_row_data[10] = {0};
    u32tou8Handle_t l_flashaddrTobytes;
    u32tou8Handle_t input_data_segg;

    //printf("BOOTLOADER_PARTION_SIZE: ");
    //scanf("%d",&input_data_segg.u32data);
    input_data_segg.u32data = 0x12345678;
    
    l_flashaddrTobytes.u32data = 0x8010000;
    l_flashaddrTobytes = reverse(l_flashaddrTobytes);
    input_data_segg = reverse(input_data_segg);
    
    for(int i = 0;i <= (l_data_length + 5);i++,j++)
    {
        if(i < 4)
        {
            l_row_data[j] = l_flashaddrTobytes.bytes[i];
        }
        else if(i == 4)
        {
            l_row_data[j] = l_data_length;
        }else if(i > 4)
        {
        l_row_data[j] = input_data_segg.bytes[i - 5];
        }
    }
    //print in the file
    char t = ' ';
    fprintf(ptr_txt_file, "%c",t);//Writing space before the starting byte
    for(int i = 0;i<9;i++)
        fprintf(ptr_txt_file, "%x ", l_row_data[i]);//writing the actual data
    fprintf(ptr_txt_file,"\n");
    fclose(ptr_txt_file);
    //------------------------------------------//

}