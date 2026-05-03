with Interfaces.C;
with Interfaces.C.Strings;
with System;

package Hesia_Core is
   pragma Linker_Options ("-lhesia_core");

   subtype Status is Interfaces.C.int;

   Status_Ok                    : constant Status := 0;
   Status_Invalid_Argument      : constant Status := 1;
   Status_Buffer_Too_Small      : constant Status := 2;
   Status_Unsupported           : constant Status := 3;
   Status_Crypto_Error          : constant Status := 4;
   Status_Authentication_Failed : constant Status := 5;
   Status_Policy_Denied         : constant Status := 6;
   Status_Watchdog_Expired      : constant Status := 7;
   Status_Inference_Rejected    : constant Status := 8;
   Status_IO_Error              : constant Status := 9;
   Status_Internal_Error        : constant Status := 10;
   Status_Maintenance_Required : constant Status := 11;

   subtype Sig_Algorithm is Interfaces.C.int;
   Sig_Ed25519    : constant Sig_Algorithm := 1;
   Sig_ML_DSA_65  : constant Sig_Algorithm := 2;
   Sig_ML_DSA_87  : constant Sig_Algorithm := 3;

   type Error is record
      Status_Field : Status;
      Subsystem    : Interfaces.C.char_array (0 .. 31);
      Message      : Interfaces.C.char_array (0 .. 191);
   end record
     with Convention => C;

   type Maintenance_Info is record
      Firmware_Version_Packed : Interfaces.C.unsigned;
      Security_Patch_Level    : Interfaces.C.unsigned;
      Last_Maintenance_Unix   : Interfaces.C.unsigned_long_long;
      Tamper_Detected         : Interfaces.C.int;
      Debug_Mode_Active       : Interfaces.C.int;
   end record
     with Convention => C;

   type Watchdog is record
      Timeout_Ms        : Interfaces.C.unsigned_long_long;
      Last_Heartbeat_Ms : Interfaces.C.unsigned_long_long;
      Max_Missed        : Interfaces.C.unsigned;
      Missed            : Interfaces.C.unsigned;
      Armed             : Interfaces.C.int;
   end record
     with Convention => C;

   function Version return Interfaces.C.Strings.chars_ptr
     with Import, Convention => C, External_Name => "hesia_version";

   function Status_String (Code : Status) return Interfaces.C.Strings.chars_ptr
     with Import, Convention => C, External_Name => "hesia_status_string";

   procedure Error_Init (Err : access Error)
     with Import, Convention => C, External_Name => "hesia_error_init";

   function Error_Message (Err : access constant Error) return Interfaces.C.Strings.chars_ptr
     with Import, Convention => C, External_Name => "hesia_error_message";

   function Random
     (Output : System.Address;
      Length : Interfaces.C.size_t;
      Err    : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_random";

   -- Signature API
   function Sig_Public_Key_Bytes
     (Alg : Sig_Algorithm;
      Len : access Interfaces.C.size_t;
      Err : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_sig_public_key_bytes";

   function Sig_Secret_Key_Bytes
     (Alg : Sig_Algorithm;
      Len : access Interfaces.C.size_t;
      Err : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_sig_secret_key_bytes";

   function Sig_Keypair
     (Alg    : Sig_Algorithm;
      PK     : System.Address;
      PK_Len : Interfaces.C.size_t;
      SK     : System.Address;
      SK_Len : Interfaces.C.size_t;
      Err    : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_sig_keypair";

   -- Maintenance API
   function Maintenance_Get_Info
     (Info : access Maintenance_Info;
      Err  : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_maintenance_get_info";

end Hesia_Core;
