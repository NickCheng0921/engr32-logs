from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import PeftModel
import torch

# Load base AWQ model and tokenizer
base_model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen2.5-Coder-7B-Instruct-AWQ")
tokenizer = AutoTokenizer.from_pretrained("Qwen/Qwen2.5-Coder-7B-Instruct-AWQ", trust_remote_code=True)

# Load LoRA adapter
model = PeftModel.from_pretrained(base_model, "qwen7b")

# Set to evaluation mode
model.eval()

# Move to GPU if available
device = "cuda" if torch.cuda.is_available() else "cpu"
model = model.to(device)

# ==== Prompting ====
prompt = """### System:\n\
You are an expert SystemC designer.
Write a SystemC Module for the following specification.

### Prompt:\n\
## Specification for `Double Barrel Shift`

## Interfaces
### System Components
- **Clock/Reset**
  - `sc_in<bool> sys_clk`
  - `sc_in<bool> rst_n`

### Channels
- `Connections::In<Data1> data_i`
- `Connections::Out<Data1> data_o`

## High Level Operation
 - Input data has barrel shift operations performed on it
     - Detailed in barrel shift section
 - Final data is then sent to output channel

## Barrel Shift Operation
 - Barrel shift address by 16 bits left
 - Barrel shift data by 16 bits right

## Struct and Constant Definitions
```cpp
struct Data1 {{
    ac_int<64, false> address;
    ac_int<64, false> data;
}};
```
"""
#prompt = "### Prompt:\nWhat is a function in Python?\n\n### Response:"
inputs = tokenizer(prompt, return_tensors="pt").to(device)

with torch.no_grad():
    outputs = model.generate(
        **inputs,
        max_new_tokens=2048,
        temperature=0.7,
        do_sample=True,
        pad_token_id=tokenizer.eos_token_id,
    )

response = tokenizer.decode(outputs[0], skip_special_tokens=True)

print(response)
