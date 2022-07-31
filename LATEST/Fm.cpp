#include "Std_Types.hpp"

static uint8 Ascii2Hex(uint8 c){
        if(c >= '0' && c <= '9'){return (uint8)(c - '0');}
   else if(c >= 'A' && c <= 'F'){return (uint8)(c - 'A' + 10);}
   else if(c >= 'a' && c <= 'f'){return (uint8)(c - 'a' + 10);}
   else                         {return 0;}
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

static uint8 GetByte(void){
   uint8  u8Byte  = (Ascii2Hex(GetNibble()));
          u8Byte  = (u8Byte << 4) & 0xF0;
          u8Byte |= Ascii2Hex(GetNibble());
   return u8Byte;
}

#include "Fm.hpp"

void Fm::Read(
   uint8* au8Buffer
){
   file = fopen("Fls.hex", "rb");
   if(NULL != file){
      char    chRead;
      do{
         chRead = fgetc(file);
         if(':' == chRead){
            uint8 u8CountData  = GetByte();
            uint8 u8CheckSum   = u8CountData;
            uint8 u8AddressMSB = GetByte();
                  u8CheckSum  += u8AddressMSB;
            uint8 u8AddressLSB = GetByte();
                  u8CheckSum  += u8AddressLSB;
            uint8 u8TypeRecord = GetByte();
                  u8CheckSum  += u8TypeRecord;
   
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
                  au8Buffer[u16Address + u32IndexData] = u8ByteRead;
                  u8CheckSum  += u8ByteRead;
                  printf(" %2.2X", u8ByteRead);
               }
               if(u8CheckSum != GetByte()){
                  printf("\nChecksum is corrupt!");
               }
            }
         }
      }while(EOF != chRead);
   }
   else{
      printf("Unable to open file");
   }
   printf("\n");
   fclose(file);
}
