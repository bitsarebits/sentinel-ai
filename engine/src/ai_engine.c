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

float ai_engine_predict(const PacketFeatures *features)
{
    // TODO: You implement this part!
    // 1. Convert PacketFeatures to normalized float array[5]
    // 2. Create Input Tensor
    // 3. Run Inference
    // 4. Calculate MSE (Input vs Output)
    return 0.0f;
}
/*
**Your Task:**
1.  Create `include/ai_engine.h`.
2.  Start `src/ai_engine.c` with the init/cleanup code.
3.  **Try to write the logic for `ai_engine_predict`**.
    * *Hint:* To normalize: `val = (val - min) / (max - min)`.
    * *Hint:* `OrtCreateTensorWithDataAsOrtValue` is the function to create the input tensor.

Go ahead and start coding! If you get stuck on the specific ONNX functions for prediction, just ask. */