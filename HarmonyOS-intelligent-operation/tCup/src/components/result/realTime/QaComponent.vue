<template>
  <div class="qa-container">
    <div class="input__wrap">
      <div class="input__outer">
        <div class="shadow__input"></div>
        <button class="icon-button" @click="submitQuestion" aria-label="search">
          <svg viewBox="0 0 24 24" width="18" height="18" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M21 21l-4.35-4.35" stroke="#0d2e2b" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <circle cx="11" cy="11" r="6" stroke="#0d2e2b" stroke-width="2" />
          </svg>
        </button>
        <div class="input__pill">
          <input type="text" name="text" class="input__search" placeholder="请输入您的问题..." v-model="question" @keyup.enter="submitQuestion">
        </div>
      </div>
    </div>
    <div class="qa-history" v-if="qaHistory.length > 0">
      <div class="qa-item" v-for="(item, index) in qaHistory" :key="index">
        <div class="question">Q: {{ item.question }}</div>
        <div class="answer">A: {{ item.answer }}</div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      question: "",
      qaHistory: [],
      loading: false
    };
  },
  methods: {
    submitQuestion() {
      if (!this.question.trim() || this.loading) return;
      this.loading = true;
      this.$http.post('/api/qa', { question: this.question })
        .then(response => {
          this.qaHistory.push(response);
          this.question = "";
        })
        .catch(error => {
          console.error('问答请求失败:', error);
          this.$message && this.$message.error && this.$message.error('获取回答失败，请重试');
        })
        .finally(() => { this.loading = false; });
    }
  }
};
</script>

<style scoped>
.qa-container {
  padding: 30px 20px;
}

/* From Uiverse.io by EddyBel */
/* container to center the search box */
.input__wrap {
  display: flex;
  justify-content: center;
  width: 100%;
}

.input__outer {
  position: relative;
  width: 86%;
  max-width: 920px;
  min-width: 420px;
  display: flex;
  align-items: center;
  padding: 22px;
  border-radius: 14px;
  background: rgba(8,10,12,0.55);
  box-shadow: 0 18px 60px rgba(2,6,8,0.6), inset 0 -2px 20px rgba(0,0,0,0.4);
  border: 1px solid rgba(255,255,255,0.03);
}

.shadow__input {
  position: absolute;
  width: 120%;
  height: 120%;
  left: -10%;
  top: -10%;
  z-index: 0;
  filter: blur(36px);
  border-radius: 16px;
  background: linear-gradient(90deg, rgba(153,156,255,0.12), rgba(0,0,0,0.05));
}

.icon-button {
  position: relative;
  z-index: 2;
  width: 44px;
  height: 44px;
  border-radius: 50%;
  border: none;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-right: 14px;
  background: linear-gradient(180deg, rgba(255,255,255,0.95), rgba(240,240,240,0.85));
  box-shadow: 0 8px 28px rgba(0,0,0,0.6), 0 8px 32px rgba(153,156,255,0.06);
  cursor: pointer;
}

.icon-button svg { transform: translateY(1px); }

.input__pill {
  flex: 1 1 auto;
  z-index: 2;
}
.input__pill .input__search {
  width: 100%;
  border-radius: 40px;
  outline: none;
  border: none;
  padding: 12px 22px;
  font-size: 16px;
  background: #ffffff;
  color: #222;
  box-shadow: 0 8px 30px rgba(255,255,255,0.08) inset;
}

.qa-history {
  margin-top: 22px;
}

.qa-item {
  margin-bottom: 12px;
  padding: 12px;
  border-radius: 8px;
  background: rgba(255,255,255,0.04);
  border: 1px solid rgba(255,255,255,0.03);
}

.question {
  font-weight: 600;
  color: #cfeff0;
  margin-bottom: 6px;
}

.answer {
  color: #bdebe9;
  white-space: pre-wrap;
}
</style>