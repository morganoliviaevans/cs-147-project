# This is the server.py used to setup lab 4 for AWS connection.

from flask import Flask, request, jsonify, render_template
import matplotlib.pyplot as plt
import os
from datetime import datetime
import csv

app = Flask(__name__)

@app.route("/")

# 1. ssh into AWS instance using public IPv4 address 3.85.208.114:
#  $ ssh -i ~/.ssh/lab4_cs147.pem ubuntu@3.85.208.114

# 2. $ cd cs-147-project

# 3. Activate the virtual environment:
#    $ source venv/bin/activate

# 4. $ export FLASK_APP=server.py

# 5. Launch server:
#  $ python3 -m flask run --host=0.0.0.0 --port=5000

def hello(): 
    # Get the temperature and humidity from the query parameters
    temp_fahrenheit = request.args.get('temp_fahrenheit', default="Unknown")
    humidity = request.args.get('humidity', default="Unknown")

    # Log formatted values to the console
    print(f"Received Data - Temperature: {temp_fahrenheit} °F, Humidity: {humidity} %")

    # Respond back to the client
    return f"Received Temperature: {temp_fahrenheit} °F, Humidity: {humidity} %"

# Route to handle POST request
@app.route('/send-time', methods=['POST'])
def receive_data():
    # JSON Data
    data = request.get_json()
    
    play_time = data.get("playTime")
    sleep_time = data.get("sleepTime")

    if play_time is not None and sleep_time is not None:
        print(f"Received Play Time: {play_time}")
        print(f"Received Sleep Time: {sleep_time}")
        return jsonify({"message": "Data received successfully"}), 200
    else:
        return jsonify({"error": "Invalid data"}), 400

if __name__ == "__main__":
    # Start Flask server
    app.run(host='0.0.0.0', port=5000)

# Graph Testing

def graph_times():
# Lists to hold playTime and sleepTime values
    play_times = []
    sleep_times = []

    # Read the CSV file
    with open('times.csv', 'r') as file:
        reader = csv.reader(file)
        next(reader)  # Skip the header row
        for row in reader:
            play_times.append(int(row[0]))
            sleep_times.append(int(row[1]))

    # Create graph
    plt.plot(play_times, label='Play Time')
    plt.plot(sleep_times, label='Sleep Time')

    # Label axes
    plt.xlabel('Time Period')
    plt.ylabel('Duration (ms)')
    plt.title('Play and Sleep Times')
    plt.legend()
    plt.show()