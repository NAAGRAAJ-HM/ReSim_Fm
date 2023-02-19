#include "Std_Types.hpp"

/******************************************************************************/
//TBD: Duplicated code, make library
/******************************************************************************/
static uint8 Ascii2Hex(uint8 c){
        if(c>='0'&&c<='9'){return(uint8)(c-'0');}
   else if(c>='A'&&c<='F'){return(uint8)(c-'A'+10);}
   else if(c>='a'&&c<='f'){return(uint8)(c-'a'+10);}
   else                   {return 0;}
}

/******************************************************************************/
//TBD: Duplicated code, make library
/******************************************************************************/
static uint8 Hex2Ascii(uint8 h){
        if(       h<= 9){return (uint8)(h+'0');}
   else if(h>=10&&h<=15){return (uint8)(h+'A'-10);}
   else                 {return '?';}
}

#include <stdio.h>
#include <stdint.h>

FILE* file = NULL;

/******************************************************************************/
//TBD: Duplicated code, make library
/******************************************************************************/
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
/******************************************************************************/
//TBD: Duplicated code, make library
/******************************************************************************/
static uint8 GetByte(void){
   uint8  u8Byte      = (Ascii2Hex(GetNibble()));
          u8Byte      = (u8Byte << 4) & 0xF0;
          u8Byte     |= Ascii2Hex(GetNibble());
          u8CheckSum += u8Byte;
   return u8Byte;
}

/******************************************************************************/
//TBD: Duplicated code, make library
/******************************************************************************/
static void PutByte(uint8 u8Byte){
   u8CheckSum += u8Byte;
   fputc(Hex2Ascii(0x0F&(u8Byte>>4)),file);
   fputc(Hex2Ascii(0x0F&(u8Byte   )),file);
}

#include "ReSimFm.hpp"
//TBD: Centralize the simulation techniques to this module
/*
   uint8 au8Data[32768];
   ReSim.Read(        au8Data);
   ReSim.Write(32768, au8Data);
*/

typedef struct{
   int version;
   int NumSamples;
   int LenSample;
}stHeaderLogInput;

typedef struct{
   FILE*            fptr;
   stHeaderLogInput Header;
   int              IndexSample;
}stMetaData;

ReSimFm    ReSim;
stMetaData MetaDataLogInput;

#include "DebugHere.hpp"
void DebugHere(
      uint8  lu8Path
   ,  uint8  lu8Strategy
   ,  int    ls32Value
){
   static sint16 ls16Count = 1;
   printf("Instance[%3.3d] Path[%3.3d]: Value = %d, code reached till here. ", ls16Count, lu8Path, ls32Value);
   if(DebugHere_Halt == lu8Strategy){
      printf("Debug strategy = HALT\n");
      while(1);
   }
   else{
      printf("Debug strategy = PROCEED\n");
      ls16Count++;
   }
}

void ReSimFm::InitFunction(void){
   MetaDataLogInput.fptr = fopen("Log_Input.csv", "r");
   if(NULL == MetaDataLogInput.fptr){
      printf("File opening failed\n");
   }
   else{
      printf("File opened\n");

      char char_dummy;
      fscanf(
            MetaDataLogInput.fptr
         ,  "%d%c%d%c%d%c"
         ,  &MetaDataLogInput.Header.version
         ,  &char_dummy
         ,  &MetaDataLogInput.Header.NumSamples
         ,  &char_dummy
         ,  &MetaDataLogInput.Header.LenSample
         ,  &char_dummy
      );

      printf(
            "version: %d NumSamples: %d LenSample: %d\n"
         ,  MetaDataLogInput.Header.version
         ,  MetaDataLogInput.Header.NumSamples
         ,  MetaDataLogInput.Header.LenSample
      );
      MetaDataLogInput.IndexSample = 0;
   }
}

void ReSimFm::DeInitFunction(void){
   fclose(MetaDataLogInput.fptr);
   printf("File closed\n");
}

void ReSimFm::MainFunction(void){
   if(
         MetaDataLogInput.Header.NumSamples
      >  MetaDataLogInput.IndexSample
   ){
      printf("Record[%2.2d]: ", MetaDataLogInput.IndexSample);
      int WordsSample[8/*LenSample*/];
      int IndexWord;
      for(
         IndexWord = 0;
         IndexWord < MetaDataLogInput.Header.LenSample;
         IndexWord++
      ){
         char char_dummy;
         fscanf(
               MetaDataLogInput.fptr
            ,  "%d%c"
            ,  &WordsSample[IndexWord]
            ,  &char_dummy
         );

         printf(
               " 0x%8.8X"
            ,  WordsSample[IndexWord]
         );
      }
      printf("\n");
      MetaDataLogInput.IndexSample++;
   }
   else{
      DeInitFunction(); //TBD: Initiate shutdown here
   }
}

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
