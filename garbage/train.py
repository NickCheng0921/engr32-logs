from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import get_peft_model, LoraConfig, TaskType
from trl import SFTConfig, SFTTrainer
from datasets import Dataset, DatasetDict
from tqdm import tqdm
import os
import random
import sys

# ==== Load examples ====
code_path = "../synthetic_code/"
spec_path = "../synthetic_spec/"
examples = []
for filename in tqdm(os.listdir(code_path)):
    code, spec = None, None
    with open(code_path + filename, "r") as f:
        code = f.read()
    with open(spec_path + filename[:-3] + "txt", "r") as f:
        spec = f.read()
    if not code or not spec:
        print("FAIL to read")
        sys.exit(1)
    examples.append({"prompt": spec, "output": code})

# ==== Train/val split ====
random.shuffle(examples)
split_idx = int(len(examples) * 0.9)
train_data = examples[:split_idx]
val_data = examples[split_idx:]

system_prompt = """You are an expert SystemC designer.
Write a SystemC Module for the following specification."""

# Format for instruction tuning
def format_example(ex):
    return {
        "text": f"### System:\n{system_prompt}\n\n### Prompt:\n{ex['prompt']}\n\n### Response:\n{ex['output']}"
    }

train_dataset = Dataset.from_list([format_example(e) for e in train_data])
val_dataset = Dataset.from_list([format_example(e) for e in val_data])
dataset = DatasetDict({"train": train_dataset, "validation": val_dataset})

print(f"Len train: {len(train_dataset)}, Len val: {len(val_dataset)}")

# ==== Load model ====
model_name = "Qwen/Qwen2.5-Coder-7B-Instruct-AWQ"
model = AutoModelForCausalLM.from_pretrained(model_name)
tokenizer = AutoTokenizer.from_pretrained(model_name, trust_remote_code=True)

# ==== LoRA config ====
peft_config = LoraConfig(
    task_type=TaskType.CAUSAL_LM,
    r=8,
    lora_alpha=16,
    lora_dropout=0.05,
    bias="none",
    target_modules=["q_proj", "v_proj"]  # update based on model structure
)

model = get_peft_model(model, peft_config)


# ==== Training args ====
training_args = SFTConfig(
    output_dir="/tmp",
    max_length=2048,
    per_device_train_batch_size=2,
    per_device_eval_batch_size=1,
    gradient_accumulation_steps=16,
    num_train_epochs=10,
    evaluation_strategy="epoch",
    save_strategy="no",
    fp16=True,
)

# ==== SFTTrainer ====
trainer = SFTTrainer(
    model=model,
    args=training_args,
    train_dataset=dataset["train"],
    eval_dataset=dataset["validation"],
)

trainer.train()

trainer.save_model("qwen7b")