<template>
  <div id="app" class="container">
    <div class="pattern-bg"></div>
    <div class="content-wrapper">
      <router-view/>
    </div>
    <svg
      preserveAspectRatio="xMidYMid slice"
      height="100%"
      width="100%"
      class="cube-svg"
      viewBox="0 0 120 104"
      style="pointer-events:none;"
    >
      <defs>
        <linearGradient y2="100%" x2="100%" y1="0%" x1="0%" id="cube-dark">
          <stop stop-color="#232526" offset="0%"></stop>
          <stop stop-color="#414345" offset="100%"></stop>
        </linearGradient>
        <linearGradient y2="0%" x2="100%" y1="100%" x1="0%" id="cube-mid">
          <stop stop-color="#4b6cb7" offset="0%"></stop>
          <stop stop-color="#182848" offset="100%"></stop>
        </linearGradient>
        <linearGradient y2="100%" x2="0%" y1="0%" x1="100%" id="cube-light">
          <stop stop-color="#a8edea" offset="0%"></stop>
          <stop stop-color="#fed6e3" offset="100%"></stop>
        </linearGradient>
      </defs>
    </svg>
  </div>
</template>

<script>
  export default {
    name: 'App'
  }
</script>

<style>
  .container {
    background: #181c21;
    width: 100vw;
    height: 100vh;
    margin: 0;
    position: fixed;
    left: 0;
    top: 0;
    overflow: hidden;
  }
  .pattern-bg {
    width: 100vw;
    height: 100vh;
    overflow: hidden;
    background: repeating-linear-gradient(
      135deg,
      #232526 0px,
      #232526 60px,
      #23252699 70px,
      #414345 130px
    );
    position: fixed;
    left: 0;
    top: 0;
    z-index: 0;
  }
  .cube-svg {
    position: fixed;
    width: 200vw;
    height: 200vh;
    left: -30vw;
    top: -20vh;
    background: transparent;
    opacity: 0.7;
    z-index: 1;
    animation: cubeMove 18s linear infinite alternate;
    pointer-events: none;
  }
  .content-wrapper {
    position: relative;
    z-index: 2;
    width: 100vw;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
  }
  @keyframes cubeMove {
    from {
      transform: translateY(0) scale(1);
    }
    to {
      transform: translateY(-20%) scale(1.02) rotate(1deg);
    }
  }
  /* Neon / kylin theme additions */
  /* Top navigation buttons look */
  .header-menu .header-item {
    border: 2px solid rgba(18,240,224,0.18);
    padding: 10px 26px;
    margin: 6px 12px;
    border-radius: 6px;
    color: #cfeff0;
    text-shadow: 0 1px 0 rgba(0,0,0,0.6);
    transition: all 0.18s ease-in-out;
    background: linear-gradient(180deg, rgba(255,255,255,0.02), rgba(0,0,0,0.2));
    box-shadow: 0 4px 24px rgba(18,240,224,0.03);
  }
  .header-menu .header-item:hover {
    transform: translateY(-2px);
    box-shadow: 0 8px 40px rgba(18,240,224,0.12);
    border-color: rgba(18,240,224,0.4);
  }
  .header-menu .el-menu-item.is-active, .header-menu .header-item.is-active {
    background: rgba(18,240,224,0.95);
    color: #071719 !important;
    border-color: rgba(18,240,224,0.95) !important;
    box-shadow: 0 10px 40px rgba(18,240,224,0.2);
  }

  /* Sidebar neon style */
  .el-menu--vertical {
    background: transparent !important;
    border-right: 1px solid rgba(255,255,255,0.03);
  }
  .el-menu--vertical .el-menu-item {
    color: #d9eaea;
    padding-left: 22px;
    transition: all 0.18s ease;
  }
  .el-menu--vertical .el-menu-item:hover {
    background: rgba(255,255,255,0.02);
    color: #bff7f3;
  }
  .el-menu--vertical .el-menu-item.is-active {
    background: linear-gradient(90deg, rgba(10,16,18,0.45), rgba(10,16,18,0.25));
    color: #12f0e0 !important;
    box-shadow: inset 6px 0 18px rgba(18,240,224,0.06), 0 6px 30px rgba(18,240,224,0.04);
    border-left: 4px solid rgba(18,240,224,0.9);
  }

  /* Cards and panels: translucent dark */
  .el-card, .card, .panel {
    background: rgba(8,10,12,0.55) !important;
    border: 1px solid rgba(18,240,224,0.06) !important;
    box-shadow: 0 10px 30px rgba(2,6,8,0.6), 0 6px 30px rgba(18,240,224,0.02) !important;
    color: #e6f7f6;
  }

  /* Tables & dialog backgrounds semi-transparent to show background pattern */
  .el-table, .el-dialog__body, .el-card__body {
    background: rgba(6,8,10,0.45) !important;
  }

  /* Make text slightly brighter */
  body, .el-main, .el-menu, .el-card {
    color: #dbeff0;
  }
</style>

