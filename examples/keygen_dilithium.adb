with Ada.Text_IO;
with Interfaces.C;
with Hesia_Core;
with System;

procedure Keygen_Dilithium_Ada is
   use type Interfaces.C.int;
   use type Interfaces.C.size_t;

   Err    : aliased Hesia_Core.Error;
   Result : Hesia_Core.Status;
   
   PK_Len : aliased Interfaces.C.size_t := 0;
   SK_Len : aliased Interfaces.C.size_t := 0;
   
   Alg    : constant Hesia_Core.Sig_Algorithm := Hesia_Core.Sig_ML_DSA_65;
begin
   Ada.Text_IO.Put_Line ("HESIA CoreLib Ada Demo - ML-DSA Keygen");
   
   Hesia_Core.Error_Init (Err'Access);
   
   -- 1. Get required buffer sizes
   Result := Hesia_Core.Sig_Public_Key_Bytes (Alg, PK_Len'Access, Err'Access);
   if Result /= Hesia_Core.Status_Ok then
      Ada.Text_IO.Put_Line ("Error getting PK size");
      return;
   end if;

   Result := Hesia_Core.Sig_Secret_Key_Bytes (Alg, SK_Len'Access, Err'Access);
   
   Ada.Text_IO.Put_Line ("Algorithm: ML-DSA-65");
   Ada.Text_IO.Put_Line ("PK Size:" & PK_Len'Image);
   Ada.Text_IO.Put_Line ("SK Size:" & SK_Len'Image);

   declare
      type Byte_Array is array (Interfaces.C.size_t range <>) of Interfaces.C.unsigned_char
        with Convention => C;
      
      PK : Byte_Array (1 .. PK_Len);
      SK : Byte_Array (1 .. SK_Len);
   begin
      -- 2. Generate Keypair
      Ada.Text_IO.Put_Line ("Generating keypair...");
      Result := Hesia_Core.Sig_Keypair (
         Alg, 
         PK'Address, 
         PK_Len, 
         SK'Address, 
         SK_Len, 
         Err'Access
      );
      
      if Result = Hesia_Core.Status_Ok then
         Ada.Text_IO.Put_Line ("Success! Dilithium keys generated.");
      else
         Ada.Text_IO.Put_Line ("Generation failed.");
      end if;
   end;
   
end Keygen_Dilithium_Ada;
