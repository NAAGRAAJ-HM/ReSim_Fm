#include "Std_Types.hpp"

static uint8 Ascii2Hex(uint8 c){
        if(c>='0'&&c<='9'){return(uint8)(c-'0');}
   else if(c>='A'&&c<='F'){return(uint8)(c-'A'+10);}
   else if(c>='a'&&c<='f'){return(uint8)(c-'a'+10);}
   else                   {return 0;}
}

static uint8 Hex2Ascii(uint8 h){
        if(       h<= 9){return (uint8)(h+'0');}
   else if(h>=10&&h<=15){return (uint8)(h+'A'-10);}
   else                 {return '?';}
}

#include <stdio.h>
#include <stdint.h>

FILE* file = NULL;

static uint8 GetNibble(void){
   uint8 chRead;

   do{
      chRead = fgetc(file);
   }while(
         '\n' == chRead
      || '\r' == chRead
      || ':'  == chRead
   );

   return chRead;
}

uint8 u8CheckSum;
static uint8 GetByte(void){
   uint8  u8Byte      = (Ascii2Hex(GetNibble()));
          u8Byte      = (u8Byte << 4) & 0xF0;
          u8Byte     |= Ascii2Hex(GetNibble());
          u8CheckSum += u8Byte;
   return u8Byte;
}

static void PutByte(uint8 u8Byte){
   u8CheckSum += u8Byte;
   fputc(Hex2Ascii(0x0F&(u8Byte>>4)),file);
   fputc(Hex2Ascii(0x0F&(u8Byte   )),file);
}

#include "ReSimFm.hpp"
void ReSimFm::Read(
   uint8* au8Buffer
){
   file = fopen("Fls.hex", "rb");
   if(NULL != file){
      char    chRead;
      do{
         chRead = fgetc(file);
         if(':' == chRead){
                  u8CheckSum   = 0;
            uint8 u8CountData  = GetByte();
            uint8 u8AddressMSB = GetByte();
            uint8 u8AddressLSB = GetByte();
            uint8 u8TypeRecord = GetByte();
            if(
                  (0 != u8CountData)
               && (0 == u8TypeRecord)
            ){
               uint16 u16Address = ((uint16)u8AddressMSB << 8) | u8AddressLSB;
               printf("\n[%8.8X]", u16Address);
               for(
                  uint32 u32IndexData = 0;
                         u32IndexData < u8CountData;
                         u32IndexData ++
               ){
                  uint8 u8ByteRead = GetByte();
                  au8Buffer[u16Address+u32IndexData] = u8ByteRead;
                  printf(" %2.2X", u8ByteRead);
               }
               uint8 u8CheckSumStored = GetByte();
               if(0 != u8CheckSum){
                  printf("\nChecksum is corrupt!");
                  printf("\nCalculated Checksum: 0x%2.2X", u8CheckSum);
                  printf("\nStored Checksum:     0x%2.2X", u8CheckSumStored);
               }
            }
         }
      }while(EOF != chRead);
   }
   else{
      printf("\nUnable to open file");
   }
   printf("\n");
   fclose(file);
}

void ReSimFm::Write(
      uint16 u16Size
   ,  uint8* au8Buffer
){
   file = fopen("Output.hex", "wb");
   if(NULL != file){
      for(
         uint16 u16Address  = 0;
                u16Address  < u16Size;
                u16Address += 0x10
      ){
         fputc('\n', file);
         fputc(':', file);
         u8CheckSum = 0;
         PutByte(0x10);
         PutByte(0xFF&(u16Address>>8));
         PutByte(0xFF&(u16Address   ));
         PutByte(0x00);
         for(
            uint32 u32IndexData = 0;
                   u32IndexData < 0x10;
                   u32IndexData ++
         ){
            PutByte(au8Buffer[u16Address+u32IndexData]);
         }
         PutByte(0-u8CheckSum);
      }
      fputc('\n', file);
      fputc(EOF, file);
   }
   else{
      printf("\nUnable to open file");
   }
   fclose(file);
}

void ReSimFm::CsvReadOut(void){
   FILE* ptrDataSet = fopen("DataSet.csv", "r");
   if(NULL == ptrDataSet){
      printf("\nFile opening failed");
   }
   else{
      printf("\nFile opened");
      int version;
      int DataLength;
      int RecordLength;
      char c;
      fscanf(ptrDataSet, "%d%c%d%c%d%c", &version, &c, &DataLength, &c, &RecordLength, &c);
      printf("\nversion: %d\nDataLen: %d\nRecordLength: %d", version, DataLength, RecordLength);

      int DataCount;
      for(
         DataCount = 0;
         DataCount < DataLength;
         DataCount++
      ){
         printf("\nRecord[%2.2d]: ", DataCount);
         int Record[8/*RecordLen*/];
         int RecordCount;
         for(
            RecordCount = 0;
            RecordCount < RecordLength;
            RecordCount++
         ){
            fscanf(ptrDataSet, "%d%c", &Record[RecordCount], &c);
            printf(" 0x%8.8X", Record[RecordCount]);
         }
      }

      fclose(ptrDataSet);
      printf("\nFile closed\n");
   }
}
