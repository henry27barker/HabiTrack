from flask import Flask, render_template, request
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app)

# Serve the frontend webpage
@app.route("/")
def index():
    return render_template("index.html")  # Make sure index.html exists and handles the socket event

# Receive elapsed time and send to the front-end
@app.route("/data", methods=["GET", "POST"])
def receive_data():
    if request.method == "GET":
        elapsed_time = request.args.get("elapsedTime")
    else:
        data = request.json if request.is_json else request.form
        elapsed_time = data.get("elapsedTime")

    if elapsed_time:
        print(f"Received Elapsed Time: {elapsed_time} ms")
        
        # Send data to the front-end via WebSockets
        socketio.emit("elapsed_time_update", {"elapsedTime": elapsed_time})

        return {"status": "success", "elapsedTime": elapsed_time}, 200
    else:
        return {"status": "error", "message": "Missing elapsedTime parameter"}, 400

if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)


