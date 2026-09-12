/**
 * ai_engine.h
 *
 * Interface for the ONNX Runtime Inference Engine.
 * Responsibilities:
 * - Initialize the ONNX environment and session.
 * - Load the pre-trained .onnx model.
 * - Perform Min-Max scaling on input features.
 * - Run inference and calculate reconstruction error.
 */

#ifndef AI_ENGINE_H
#define AI_ENGINE_H

#include "collector.h"

/**
 * @brief Initializes the AI Engine.
 * Loads the ONNX model from disk and prepares the runtime session.
 *
 * @param model_path Path to the .onnx file (e.g., "models/sentinel.onnx")
 * @return int 0 on success, -1 on failure.
 */
int ai_engine_init(const char *model_path);

/**
 * @brief Runs the Anomaly Detection Logic on a single packet.
 *
 * Workflow:
 * 1. Normalize features using hardcoded Min/Max values.
 * 2. Create ONNX Input Tensor.
 * 3. Run Inference (Autoencoder).
 * 4. Calculate MSE (Mean Squared Error) between Input and Output.
 *
 * @param features Pointer to the extracted packet features.
 * @return float The Anomaly Score (MSE). -1.0f on Error.
 */
float ai_engine_predict(const PacketFeatures *features);

/**
 * @brief Cleans up ONNX resources (Session, Env, Allocators).
 */
void ai_engine_cleanup(void);

#endif