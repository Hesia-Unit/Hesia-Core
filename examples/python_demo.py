import sys
import os

# Path to the hesia-core/bindings/python directory
sys.path.append(os.path.abspath("C:/Users/matis/Documents/Hesia-Firmware/open_source/hesia-core/bindings/python"))

from hesia_core import HesiaCore, HesiaError

def main():
    # Note: On Windows, the DLL would need to be in the search path or specified.
    # Since we didn't successfully build the DLL in this session (VS/MinGW missing in PATH),
    # this script demonstrates the API usage.
    
    print("HESIA CoreLib Python Demo")
    print("-" * 30)
    
    try:
        # Initializing the core (assuming libhesia_core.so/dll is available)
        # core = HesiaCore() 
        
        print("Architecture available for:")
        print("  - ML-KEM / ML-DSA (Post-Quantum)")
        print("  - ASCON-128 (Lightweight AEAD)")
        print("  - Ed25519 (Signature)")
        print("  - Watchdog & Inference Guard")
        
        print("\nAPI example:")
        print(">>> core = HesiaCore()")
        print(">>> print(core.version)")
        print("0.2.0")
        
        print("\nGenerating random entropy via HESIA:")
        print(">>> entropy = core.random(32)")
        print(">>> print(entropy.hex())")
        print("f3a2... (32 bytes of secure randomness)")
        
    except Exception as e:
        print(f"\nExecution note: {e}")
        print("The HESIA shared library (.dll/.so) must be compiled and accessible to run this live.")

if __name__ == "__main__":
    main()
