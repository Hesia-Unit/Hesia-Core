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

   type Error is record
      Status_Field : Status;
      Subsystem    : Interfaces.C.char_array (0 .. 31);
      Message      : Interfaces.C.char_array (0 .. 191);
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

   function Feature_Available (Feature : Interfaces.C.int) return Interfaces.C.int
     with Import, Convention => C, External_Name => "hesia_feature_available";

   procedure Error_Init (Err : access Error)
     with Import, Convention => C, External_Name => "hesia_error_init";

   function Error_Message (Err : access constant Error) return Interfaces.C.Strings.chars_ptr
     with Import, Convention => C, External_Name => "hesia_error_message";

   function Random
     (Output : System.Address;
      Length : Interfaces.C.size_t;
      Err    : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_random";

   function Watchdog_Init
     (W          : access Watchdog;
      Timeout_Ms : Interfaces.C.unsigned_long_long;
      Max_Missed : Interfaces.C.unsigned;
      Err        : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_watchdog_init";

   function Watchdog_Arm
     (W   : access Watchdog;
      Err : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_watchdog_arm";

   function Watchdog_Heartbeat
     (W   : access Watchdog;
      Err : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_watchdog_heartbeat";

   function Watchdog_Check
     (W   : access Watchdog;
      Err : access Error) return Status
     with Import, Convention => C, External_Name => "hesia_watchdog_check";

   procedure Watchdog_Disarm (W : access Watchdog)
     with Import, Convention => C, External_Name => "hesia_watchdog_disarm";

end Hesia_Core;
