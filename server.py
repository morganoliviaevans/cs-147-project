# This is the server.py used to setup the project for AWS connection

# 1. ssh into AWS instance using public IPv4 address 3.85.208.114:
#    $ ssh -i ~/.ssh/lab4_cs147.pem ubuntu@3.85.208.114

# 2. $ cd cs-147/project

# 3. Activate the virtual environment:
#    $ source venv/bin/activate

# 4. Install dependencies:
#    $ pip install flask matplotlib
#    $ pip install requests

# 5. $ export FLASK_APP=server.py

# 6. Launch server:
#    $ python3 -m flask run --host=0.0.0.0 --port=5000

# 7. View Data Plot
#    $ curl http://3.85.208.114:5000/graph

from flask import Flask, request, jsonify, send_file
import matplotlib.pyplot as plt
import os
import csv
import requests
from threading import Thread

app = Flask(__name__)

# Path to the CSV file
csv_file = 'times.csv'

# Ensure the CSV file exists or create it with a header
if not os.path.exists(csv_file):
    with open(csv_file, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['Play Time', 'Sleep Time'])  # Add header row

# Define the graphing function first
def graph_times():
    play_times = []
    sleep_times = []

    # Read data from CSV
    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        next(reader)  # Skip the header row
        for row in reader:
            play_times.append(int(row[0]))
            sleep_times.append(int(row[1]))

    # Convert times to minutes for graphing
    play_times_minutes = [x / 60000 for x in play_times]
    sleep_times_minutes = [x / 60000 for x in sleep_times]

    # Create plot
    plt.plot(play_times_minutes, label='Play Time (minutes)', color='dodgerblue', linewidth=2)
    plt.plot(sleep_times_minutes, label='Sleep Time (minutes)', color='darkorange', linewidth=2)

    # Label axes
    plt.xlabel('Time Period')
    plt.ylabel('Duration (minutes)')
    plt.title('Play and Sleep Times')
    plt.legend()
    # plt.show()
    plt.savefig('graph.png')

@app.route("/")
def home():
    return "Flask server is running. Use /graph to display play and sleep time data plot."

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

        # Append data to CSV file
        with open(csv_file, 'a', newline='') as file:
            writer = csv.writer(file)
            writer.writerow([play_time, sleep_time])

        return jsonify({"message": "Data received successfully"}), 200
    else:
        return jsonify({"error": "Invalid data"}), 400

# Play and Sleep Time Data Plots

@app.route('/graph')
def display_graph():
    try:
        # Display plot
        graph_times()
        # Return the graph image (graph.png)
        return send_file('graph.png', mimetype='image/png')
    except FileNotFoundError:
        return "Error: times.csv not found. Make sure data has been received.", 400
    except Exception as e:
        return f"Error generating graph: {str(e)}", 500

# Main
if __name__ == "__main__":
    # Start Flask server
    app.run(host='0.0.0.0', port=5000)