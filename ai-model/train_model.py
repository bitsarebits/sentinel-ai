import pandas as pd
import numpy as np
import torch
import torch.nn as nn
from sklearn.preprocessing import MinMaxScaler

# --- REPRODUCIBILITY SETUP ---
# This ensures the model starts with the exact same weights every time.
torch.manual_seed(42)

# --- DATA PREPROCESSING ---

# Load Data
df = pd.read_csv("data/training_data.csv")

print(f"✅ Dataset Loaded. Shape: {df.shape}")
print("\n--- Sample Data ---")
print(df.head())

print("\n--- Statistics ---")
print(df.describe())

# Check for NaNs (Empty values)
if df.isnull().values.any():
    print("⚠️  Warning: Dataset contains NaN values!")
else:
    print("✅ Data is clean (No NaNs).")


# Drop the timestamp column
del df["timestamp"]

# Normalization
scaler = MinMaxScaler()
scaled_data = pd.DataFrame(scaler.fit_transform(df))
print("\n--- Scaled Data ---")
print(scaled_data.head())

print("\n--- Statistics ---")
print(scaled_data.describe())

# --- MODEL DEFINITION ---


# Prepare the neural network
class Autoencoder(nn.Module):
    def __init__(self):
        super().__init__()
        # Define the layers
        self.encoder = nn.Sequential(
            nn.Linear(5, 4),
            nn.ReLU(),
            nn.Linear(4, 3),
            nn.ReLU(),
        )
        self.decoder = nn.Sequential(
            nn.Linear(3, 4),
            nn.ReLU(),
            nn.Linear(4, 5),
            nn.Sigmoid(),
        )

    def forward(self, x):
        # Define how data flows through the layers
        x = self.encoder(x)
        x = self.decoder(x)
        return x


# --- TRAINING ---

print("\n --- Training ---\n")
# model instance
model = Autoencoder()

# Loss function (the critic)
criterion = nn.MSELoss()

# Optimizer (the mechanic)
optimizer = torch.optim.Adam(model.parameters(), lr=0.001)

# Tensor (the fuel)
inputs = torch.tensor(scaled_data.values, dtype=torch.float32)

# Training (PyTorch Waltz)
for epoch in range(20000):
    output = model(inputs)
    loss = criterion(output, inputs)  # Compare prediction with reality
    optimizer.zero_grad()  # Clear old calculations
    loss.backward()  # Calculate new corrections (backpropagation)
    optimizer.step()  # Apply corrections
    if (epoch + 1) % 10 == 0:
        print(f"Loss item epoch {epoch +1}")
        print(loss.item())

print("\n--- Params for C Engine ---")
print("Min Values:", scaler.data_min_)
print("Max Values:", scaler.data_max_)


# --- THRESHOLD CALCULATION ---

print("\n--- Calculating Threshold ---")

model.eval()  # Switch to evaluation mode (turns off learning)

with torch.no_grad():  # Don't calculate gradients (saves RAM)
    # Get the reconstruction for the whole dataset
    reconstructions = model(inputs)

    # Calculate the squared error per packet (loss per row)
    # (input - output)^2
    loss_per_packet = torch.mean((inputs - reconstructions) ** 2, dim=1)

    # Calculate Stats
    mean_loss = torch.mean(loss_per_packet)
    std_loss = torch.std(loss_per_packet)

    print(f"Mean Loss: {mean_loss.item():.6f}")
    print(f"Std Dev:   {std_loss.item():.6f}")

    # Define the Threshold (The Alarm Line)
    # We use 3 standard deviations (covering 99.7% of normal data)
    threshold = mean_loss + (3 * std_loss)

    print(f"\n ANOMALY THRESHOLD: {threshold.item():.6f}")


# --- SAVE MODEL TO ONNX ---

print("\n--- Saving Model to ONNX ---")

# Create a dummy input (1 packet, 5 features)
dummy_input = torch.randn(1, 5, dtype=torch.float32)

# Export
torch.onnx.export(
    model,  # The trained model
    dummy_input,  # The fake data to trace the path
    "models/sentinel.onnx",  # Where to save it
    input_names=["input"],  # Name of the entrance door
    output_names=["output"],  # Name of the exit door
    dynamic_axes={  # Allow C to send 1 packet or 100 packets at once
        "input": {0: "batch_size"},
        "output": {0: "batch_size"},
    },
)

print("Model saved to models/sentinel.onnx")
