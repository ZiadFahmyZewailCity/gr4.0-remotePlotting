## Running the POC

### 1. Start the Dashboard Server

Go to the build directory of the dashboard

```bash
cd POC_configurable_dashboard/imGUI_Dashboard/build
```

Start an HTTP server to serve the dasboard files:

```bash
python3 -m http.server 8000
```

### 2. Open the Dashboard

Open a web browser and navigate to:

```text
http://localhost:8000/dashboard.html
```

The dashboard should load and wait for incoming data from the flowgraph.

### 3. Run the Flowgraph

In a separate terminal:

```bash
cd POC_Block_Flowgraph/build
./POC_flowGraph
```

### 4. What you should see

Once both the dashboard and flowgraph are running:

- A live sine wave should appear in the dashboard plot.
- Moving the frequency slider in the dashboard should change the sine wave frequency in real time.
