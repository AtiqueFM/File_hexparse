

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SET                 1
#define RESET               !SET
#define PRINT_HEX_DATA      SET

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
char filename[] = "Application_V3.0.0.hex";
/*Hex file parsed*/
char filename_MCU[] = "Read_data.txt";
/*Row hex data*/
int row_data[50];
/*length of row data*/
unsigned int length;

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

    //printf("Data type: %x\n",data_typ);
    //printf("CRC: %x\n",hex_pair[data_start+data_len]);
    
    switch(data_typ)
    {
        case DATA_RECORD:
        /*
        Actual HEX data to be sent over UART/SPI to target MCU.
        */
        #if PRINT_HEX_DATA
            printf("%x | ",flash_addr + offset_addr);
            //printf("data_len is :%d\n",data_len);
            for(int i = 0;i<data_len;i++)
            {
                //Send over uart with FLASH address first
                printf("%x ",hex_pair[data_start + i]);
                row_data[j] = hex_pair[data_start + i];
                j += 1;
            }
            length = data_len - 1;
            printf("\n");
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
	
    if (NULL == ptr_txt_file) {
		printf("file can't be opened \n");
	}
	printf("content of this file are \n");

	// Printing what is written in file
	// character by character using loop.
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
            //Write the roe hex data into new file
            int k = 0;
            for(k = 0;k<=length;k++)
                fprintf(ptr_txt_file, "%x ", row_data[k]);
            fprintf(ptr_txt_file,"\n");
            memset(row_data,NULL,sizeof(row_data));
            length = 0;
            index = 0;//test
        }
        // Checking if character is not EOF.
        // If it is EOF stop eading.
    	} while (ch != EOF);
	// Closing the file
	fclose(ptr);
	
	
	return 0;
}

