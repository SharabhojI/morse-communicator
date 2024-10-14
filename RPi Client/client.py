import os
import socket
import threading
import tkinter as tk
from tkinter import scrolledtext
from dotenv import load_dotenv, dotenv_values

# Load Environment Variables
load_dotenv()

# Configuration
arduino_ip = os.getenv("ARDUINO_IP")
arduino_port = 80
host = '0.0.0.0'  # Listen on all available network interfaces
port = 8080

# Morse Code Dictionary
MORSE_CODE_DICT = {
    'A': '.-', 'B': '-...', 'C': '-.-.', 'D': '-..', 'E': '.', 'F': '..-.',
    'G': '--.', 'H': '....', 'I': '..', 'J': '.---', 'K': '-.-', 'L': '.-..',
    'M': '--', 'N': '-.', 'O': '---', 'P': '.--.', 'Q': '--.-', 'R': '.-.',
    'S': '...', 'T': '-', 'U': '..-', 'V': '...-', 'W': '.--', 'X': '-..-',
    'Y': '-.--', 'Z': '--..', '1': '.----', '2': '..---', '3': '...--',
    '4': '....-', '5': '.....', '6': '-....', '7': '--...', '8': '---..',
    '9': '----.', '0': '-----', ' ': '/'
}

REVERSE_MORSE_DICT = {value: key for key, value in MORSE_CODE_DICT.items()}

def encode_morse(message):
    return ' '.join(MORSE_CODE_DICT.get(char.upper(), 'NIL') for char in message)

def decode_morse(morse_code):
    words = morse_code.split('/')
    decoded_words = []
    for word in words:
        letters = word.strip().split()
        decoded_word = ''.join(REVERSE_MORSE_DICT.get(letter, 'NIL') for letter in letters)
        decoded_words.append(decoded_word)
    return ' '.join(decoded_words)

def send_message(message):
    morse_code = encode_morse(message)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((arduino_ip, arduino_port))
        s.sendall(morse_code.encode() + b'\n')
    update_sent_messages(message, morse_code)

def update_sent_messages(original_message, morse_code):
    messages_text.config(state=tk.NORMAL)
    messages_text.insert(tk.END, f"Sent: '{original_message}' as '{morse_code}'\n")
    messages_text.see(tk.END)
    messages_text.config(state=tk.DISABLED)

def receive_messages():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Listening for connections on {host}:{port}...")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Connection from {addr}")
        data = client_socket.recv(1024)
        message = data.decode().strip()
        if message: # Process non-empty messages only
            update_received_messages(message)
        client_socket.close()

def update_received_messages(message):
    decoded_message = decode_morse(message)
    messages_text.config(state=tk.NORMAL)
    messages_text.insert(tk.END, f"Received: '{message}', Decoded as: '{decoded_message}'\n")
    messages_text.see(tk.END)
    messages_text.config(state=tk.DISABLED)

def on_send_button_click():
    message = message_entry.get()
    send_message(message)
    message_entry.delete(0, tk.END)

# GUI setup
root = tk.Tk()
root.title("Morse Code Communicator")

frame = tk.Frame(root)
frame.pack(pady=10)

message_label = tk.Label(frame, text="Enter message:")
message_label.pack(side=tk.LEFT)

message_entry = tk.Entry(frame, width=50)
message_entry.pack(side=tk.LEFT, padx=10)

send_button = tk.Button(frame, text="Send", command=on_send_button_click)
send_button.pack(side=tk.LEFT)

messages_text = scrolledtext.ScrolledText(root, width=60, height=20, state=tk.DISABLED)
messages_text.pack(pady=10)

# Start the receiving thread
threading.Thread(target=receive_messages, daemon=True).start()

root.mainloop()