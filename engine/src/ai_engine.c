#include "ai_engine.h"
#include <onnxruntime_c_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Global State
const OrtApi *g_ort = NULL;
OrtEnv *g_env = NULL;
OrtSession *g_session = NULL;
OrtSessionOptions *g_session_options = NULL;

// --- HARDCODED PARAMETERS (From Python Training) ---
static const float MIN_VALS[] = {0.0f, 0.0f, 0.0f, 42.0f, 0.0f};
static const float MAX_VALS[] = {17.0f, 60970.0f, 60970.0f, 64866.0f, 25.0f};

// Input/Output Names (Must match Python export)
static const char *INPUT_NAMES[] = {"input"};
static const char *OUTPUT_NAMES[] = {"output"};

int ai_engine_init(const char *model_path)
{
    // 1. Get API Handle
    g_ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!g_ort)
        return -1;

    // 2. Create Environment
    if (g_ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "SentinelAI", &g_env) != NULL)
    {
        return -1;
    }

    // 3. Create Session Options
    if (g_ort->CreateSessionOptions(&g_session_options) != NULL)
    {
        return -1;
    }

    // 4. Load Model
    // Note: ONNX C API usually expects specific cleanup on error, simplified here.
    if (g_ort->CreateSession(g_env, model_path, g_session_options, &g_session) != NULL)
    {
        fprintf(stderr, "[AI] Error loading model from %s\n", model_path);
        return -1;
    }

    printf("[AI] Model loaded successfully.\n");
    return 0;
}

void ai_engine_cleanup(void)
{
    if (g_session)
        g_ort->ReleaseSession(g_session);
    if (g_session_options)
        g_ort->ReleaseSessionOptions(g_session_options);
    if (g_env)
        g_ort->ReleaseEnv(g_env);
}

/**
 * @brief Applies Min-Max normalization to a single feature.
 *
 * Implements the formula: X_norm = (X - min) / (max - min).
 * Uses the hardcoded MIN_VALS and MAX_VALS arrays derived from the training set.
 *
 * @param feature The raw feature value (e.g., source port, protocol).
 * @param normalized_feature Pointer to the float where the result will be stored.
 * @param index The index (0-4) corresponding to the specific feature type.
 * Crucial for selecting the correct min/max limits.
 */
void normalize_data(uint16_t feature, float *normalized_feature, int index)
{
    *normalized_feature = (feature - MIN_VALS[index]) / (MAX_VALS[index] - MIN_VALS[index]);
}

float ai_engine_predict(const PacketFeatures *features)
{

    // Initialize variables
    OrtStatus *status = NULL;
    OrtMemoryInfo *memory_info = NULL;
    OrtValue *input_tensor = NULL;
    OrtValue *output_tensor = NULL;
    float mse = 0.0f;

    // Normalize the data
    float input_data[FEATURES_NUMBER];
    normalize_data(features->protocol, &input_data[0], 0);
    normalize_data(features->src_port, &input_data[1], 1);
    normalize_data(features->dest_port, &input_data[2], 2);
    normalize_data(features->packet_len, &input_data[3], 3);
    normalize_data(features->tcp_flags, &input_data[4], 4);

    // Create Memory Info
    status = g_ort->CreateCpuMemoryInfo(
        OrtArenaAllocator, // Allocator Type
        OrtMemTypeDefault, // Memory Type
        &memory_info       // Output Pointer
    );
    if (status != NULL)
        goto error;

    // Create Input Tensor
    // Define Shape: [1, 5] (1 Packet, 5 Features)
    const int64_t input_shape[] = {1, FEATURES_NUMBER};

    status = g_ort->CreateTensorWithDataAsOrtValue(
        memory_info,                         // Memory Info
        input_data,                          // Raw Data Pointer
        FEATURES_NUMBER * sizeof(float),     // Total Length in BYTES
        input_shape,                         // Dimensions Array
        2,                                   // Number of Dimensions
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, // Data Type
        &input_tensor                        // Output Pointer
    );
    if (status != NULL)
        goto error;

    // Run Inference
    // Note: We pass NULL for RunOptions to use defaults.
    // Note: We pass &input_tensor because the API expects an array of tensors.
    // Note: We cast &input_tensor to (const OrtValue* const*) to satisfy the strict API signature
    status = g_ort->Run(
        g_session,                              // The Session
        NULL,                                   // Run Options
        INPUT_NAMES,                            // Input Names Array
        (const OrtValue *const *)&input_tensor, // Input Tensors Array
        1,                                      // Number of Inputs
        OUTPUT_NAMES,                           // Output Names Array
        1,                                      // Number of Outputs
        &output_tensor                          // Output Tensors Array (Result)
    );

    // Extract Output (tensor -> float[])
    float *output_data_ptr = NULL;
    // Get pointer to the raw float array INSIDE the tensor
    status = g_ort->GetTensorMutableData(output_tensor, (void **)&output_data_ptr);
    if (status != NULL)
        goto error;

    // Calculate MSE (Anomaly Score)
    for (int i = 0; i < FEATURES_NUMBER; i++)
    {
        // (Input - Reconstructed)^2
        float diff = input_data[i] - output_data_ptr[i];
        mse += (diff * diff);
    }
    mse /= FEATURES_NUMBER;

    // Cleanup
    // Free the Output (The result of the inference)
    if (output_tensor)
        g_ort->ReleaseValue(output_tensor);

    // Free the Input (The wrapper around your data)
    if (input_tensor)
        g_ort->ReleaseValue(input_tensor);

    // Free the Configuration
    if (memory_info)
        g_ort->ReleaseMemoryInfo(memory_info);

    return mse;

end:
    // --- CLEANUP RESOURCES ---
    // Release in reverse order of creation
    if (output_tensor)
        g_ort->ReleaseValue(output_tensor);
    if (input_tensor)
        g_ort->ReleaseValue(input_tensor);
    if (memory_info)
        g_ort->ReleaseMemoryInfo(memory_info);

    return mse;

error:
    // Log the error message from ONNX
    const char *msg = g_ort->GetErrorMessage(status);
    fprintf(stderr, "[AI ENGINE] Inference Failed: %s\n", msg);
    g_ort->ReleaseStatus(status); // Must free the status object
    mse = -1.0f;                  // Return error code
    goto end;
}
