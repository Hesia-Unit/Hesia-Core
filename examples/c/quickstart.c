#include "hesia/core.h"

#include <stdio.h>

int main(void) {
    uint8_t input[4] = {1, 2, 3, 4};
    uint8_t output[16] = {0};
    hesia_error_t error;
    hesia_model_contract_t contract = {
        "demo-model",
        1,
        sizeof(input),
        sizeof(output),
        20,
        0,
        NULL,
        0
    };
    hesia_inference_request_t request = {
        input,
        sizeof(input),
        output,
        sizeof(output),
        10
    };

    hesia_error_init(&error);
    if (hesia_inference_validate(&contract, &request, &error) != HESIA_STATUS_OK) {
        fprintf(stderr, "HESIA rejected request: %s\n", hesia_error_message(&error));
        return 1;
    }

    printf("HESIA Core %s accepted the inference request\n", hesia_version());
    return 0;
}
