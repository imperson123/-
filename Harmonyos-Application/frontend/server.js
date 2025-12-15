const express = require('express');
const axios = require('axios');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();
app.use(cors());
app.use(express.json());

// Serve static frontend
app.use(express.static(path.join(__dirname, 'public')));

// Simple chat proxy endpoint
app.post('/api/chat', async (req, res) => {
  const userMessage = req.body.message || '';
  const apiKey = process.env.OPENAI_API_KEY || process.env.API_KEY;
  const model = process.env.MODEL || 'gpt-4o-mini';

  if (!apiKey) {
    return res.status(500).json({ error: 'Missing OPENAI_API_KEY environment variable.' });
  }

  try {
    const payload = {
      model,
      messages: [
        { role: 'user', content: userMessage }
      ],
      max_tokens: 800
    };

    const response = await axios.post('https://api.openai.com/v1/chat/completions', payload, {
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      }
    });

    const reply = response.data.choices?.[0]?.message?.content || '';
    res.json({ reply });
  } catch (err) {
    console.error('Chat proxy error:', err.response?.data || err.message || err);
    res.status(500).json({ error: err.response?.data || err.message });
  }
});

const port = process.env.PORT || 3000;
app.listen(port, () => console.log(`Frontend chat server running at http://localhost:${port}`));
