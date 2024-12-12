import random

def generate_value(prev_value):
    # Generate a random value to increase or decrease the previous value
    change = random.randint(-20, 20)
    # Ensure the new value is positive
    new_value = max(1, prev_value + change)
    return new_value

def generate_file(outfile, n):
    labels = ['expt', 'temperature', 'rand', 'time']
    with open(outfile, 'w') as f:
        # Initialize previous values for each label
        prev_values = {'expt': 1, 'temperature': 50, 'rand': 3, 'time': 1}
        for _ in range(n):
            # Increment time by 1
            prev_values['time'] += 1
            # Generate values for other labels
            for label in labels:
                if label != 'time':
                    prev_values[label] = generate_value(prev_values[label])
                # Write label-value pairs to the file
                f.write(f"{label}={prev_values[label]}\n")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python script.py <outfile> <n>")
        sys.exit(1)
    outfile = sys.argv[1]
    n = int(sys.argv[2])
    generate_file(outfile, n)
    print(f"Generated {n} lines in {outfile}")
