import asyncio
import json
import math
import time
import websockets

#Global variable remove later
current_frequency = 1.0
async def fakeTelemData(socket):
    config = {
        "msg_type": "config",
        "dashboard_title": "POC imGUI",
        "panels": [
            {
                "panel_name": "Telemetry Slider Check",
                "dashboardElement": [
                    {
                        "id": "label_1",
                        "type": "text",
                        "title": "Text for slider"
                    },
                    {
                        "id": "slider_1",
                        "type": "widget",
                        "title": "Slider controlled by pushed data",
                        "data_source": "static_sin_data"
                    }
                ]
            },
            {
                "panel_name": "Standard Plotting",
                "dashboardElement": [
                    {
                        "id": "plot_1",
                        "type": "timeseries",
                        "title": "Static Sine Wave",
                        "data_source": "static_sin_data"
                    }
                ]
            },
            {
                "panel_name": "Command & Control",
                "dashboardElement": [
                    {
                        "id": "label_2",
                        "type": "text",
                        "title": "Control Frequency of Plot 2"
                    },
                    {
                        "id": "slider_2",
                        "type": "widget",
                        "title": "Frequency Multiplier",
                        "data_source": "freq_slider_state" 
                    },
                    {
                        "id": "plot_2",
                        "type": "timeseries", 
                        "title": "Dynamic Sine Wave",
                        "data_source": "dynamic_sin_data"
                    }
                ]
            }
        ]
    }

    print("Sending config")
    await socket.send(json.dumps(config))

    async def payload_update():
        global current_frequency
        try:
            async for message in socket:
                inbound_data = json.loads(message)

                if inbound_data.get("msg_type") == "control" and inbound_data.get("id") == "slider_2":
                    current_frequency = float(inbound_data.get("value"))
                    print(f"Frontend commanded new frequency: {current_frequency}")
        except websockets.exceptions.ConnectionClosed:
            pass

    listening_task = asyncio.create_task(payload_update())

    print("Now pushing data")


    #All the data is bundled into a single packet here sent at a fixed rate
    #Not sure if this approach or an approach where packets are sent at different rates 
    #is better depending on the frequency required to send the data, sounds more complicated but may be needed
    t = 0.0
    try:
        
        while True:
            # The sin waves being sent
            static_sin = math.sin(t)
            dynamic_sin = math.sin(t * current_frequency)

            # The packet of data 
            data = {
                "msg_type": "telemetry",
                "timestamp": time.time(),
                "dynamic_sin_data": dynamic_sin,          
                "static_sin_data": static_sin,
                "freq_slider_state": current_frequency
            }
            
            await socket.send(json.dumps(data))
            
            t += 0.1
            # Wait a bit before sending next packet
            await asyncio.sleep(0.05)  

    except websockets.exceptions.ConnectionClosed:
        print("Disconnect occurred")
        listening_task.cancel()

async def main():
    print("Starting dummy instance for testing dashboard functionality")
    async with websockets.serve(fakeTelemData, "0.0.0.0", 9000):
        await asyncio.Future()  

if __name__ == "__main__":
    asyncio.run(main())

