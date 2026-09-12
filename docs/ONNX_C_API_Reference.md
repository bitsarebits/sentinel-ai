# ONNX Runtime C API Quick Reference
// Header: #include <onnxruntime_c_api.h>

1. Initialization
   - OrtEnv* env;
   - OrtCreateEnv(ORT_LOGGING_LEVEL_WARNING, "test", &env);

2. Session Options
   - OrtSessionOptions* session_options;
   - OrtCreateSessionOptions(&session_options);
   - OrtSessionOptionsAppendExecutionProvider_CPU(session_options, 1);

3. Loading Model
   - OrtSession* session;
   - OrtCreateSession(env, model_path, session_options, &session);

4. Inputs/Outputs
   - OrtMemoryInfo* memory_info;
   - OrtCreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info);
   - OrtValue* input_tensor = NULL;
   - OrtCreateTensorWithDataAsOrtValue(memory_info, input_data, input_data_len, input_shape, input_shape_len, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);

5. Inference
   - OrtRun(session, run_options, input_names, &input_tensor, 1, output_names, 1, &output_tensor);

6. Cleanup (Crucial!)
   - OrtReleaseValue(output_tensor);
   - OrtReleaseValue(input_tensor);
   - OrtReleaseSession(session);
   - OrtReleaseSessionOptions(session_options);
   - OrtReleaseEnv(env);
