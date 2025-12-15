import psutil

# 资源指标
cpu_times = psutil.cpu_times()
cpu_counts = psutil.cpu_count()
cpu_percent_list = []
for i in range(cpu_counts):
    cpu_percent_list.append(psutil.cpu_percent(i))
mem = psutil.virtual_memory()
memtotal = mem.total
memfree = mem.free
mempercent = mem.percent
memused = mem.used
disk_usage = psutil.disk_usage('/')
disk_io = psutil.disk_io_counters()
net_io = psutil.net_io_counters()

# 业务指标
users = psutil.users()

print("System Status:")
print("cpu时间：\t" + str(cpu_times))
print("cpu占用率：共有" + str(len(cpu_percent_list)) + "个核心，占用率分别为：")
for j, item in enumerate(cpu_percent_list):
    print("核心" + str(j) + "的占用率为：\t" + str(item) + '%')
print("内存总量：\t" + str(memtotal))
print("内存占用率：\t" + str(mempercent) + "%")
print("磁盘占用率：\t" + str(disk_usage))
print("磁盘io：\t" + str(disk_io))
print("网络io：\t" + str(net_io))
print("用户信息：\t" + str(users))

print("Finished!!!")
