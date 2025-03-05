import express from 'express';

const app = express();
const port = 5000;

// 解析JSON请求体
app.use(express.json());

app.get('/v1/temp/raw', (req, res) => {
  const data = {
    acceleration: { 
      time: new Date().toLocaleTimeString(), 
      x: (Math.random() * 2 - 1), 
      y: (Math.random() * 2 - 1), 
      z: (Math.random() * 2 - 1) 
    },
    angle: { 
      time: new Date().toLocaleTimeString(), 
      x: (Math.random() * 360), 
      y: (Math.random() * 360), 
      z: (Math.random() * 360) 
    }
  };
  res.json(data);
  console.log('Sent raw data', data);
});

app.post('/v1/light/brightness', (req, res) => {
  const controlData = req.body.data;
  console.log('Received control data:', controlData);
  res.json({ message: 'Control data received', controlData });
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});