# 導入所需的模組
import pymysql
import pandas as pd
import matplotlib.pyplot as plt

from matplotlib.font_manager import FontProperties

import matplotlib.pyplot as plt
plt.rcParams['font.sans-serif'] = ['Microsoft JhengHei'] 


# 設定資料庫連線參數
db_settings = {
    "host": "127.0.0.1",
    "port": 3333,
    "user": "swimuser",
    "password": "bcps8562620",
    "db": "swim",
    "charset": "utf8"
}

# 建立資料庫連線
conn = pymysql.connect(**db_settings)

# 查詢資料庫中的溫溼度資料
sql = "SELECT updata, heartbeat, blood_oxygen FROM people"
df = pd.read_sql(sql, conn)

# 關閉資料庫連線
conn.close()

# 設定圖表標題和軸標籤
plt.title("泳者心律偵測折線圖")
plt.xlabel("日期")
plt.ylabel("心跳 / 血氧")

# 繪製溫度和濕度的折線圖，並設定顏色和樣式
plt.plot(df["updata"], df["heartbeat"], color="red", marker="o", linestyle="--", label="心跳")
plt.plot(df["updata"], df["blood_oxygen"], color="blue", marker="s", linestyle="-", label="血氧")
# plt.plot(df["updatatime"], df["temp_water"], color="green", marker="s", linestyle="-", label="水溫度")

# 顯示圖例
plt.legend()

# 顯示圖表
plt.show()
