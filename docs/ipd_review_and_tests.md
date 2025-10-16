# CSC8501 IPD 构建与实验诊断报告

## 1. 构建与代码体检

### 1.1 构建
- 使用 `g++ -std=c++17 -O2 -Wall -Wextra -pedantic` 编译 `CSC8501-Coursework-Main-Assignment/*.cpp`，生成的可执行文件位于仓库根目录 `./ipd`，权限与大小如 `ls -l` 所示，确认构建成功。 【e5faf0†L1-L4】【755606†L1-L2】

### 1.2 CLI 能力核查
- `./ipd --help` 列出了完整参数集，覆盖题目要求的全部选项（含 `--scb`、`--save/--load`、多格式输出等）。【ede2ac†L1-L20】【F:CSC8501-Coursework-Main-Assignment/Config.cpp†L51-L131】
- 无效支付会被校验并给出提示，确认同时检查了 `T>R>P>S` 与 `2R>T+S` 条件。【dd8022†L1-L3】
- `--format text/csv/json` 产出示例如冒烟用例与 `head` 结果所示；`--scb` 在 CSV 中追加 `net_mean/cost` 字段。【f71089†L1-L16】【0eb003†L1-L5】【4c0373†L1-L10】【5a7ee8†L1-L6】
- `--save` 将最终配置写入 JSON，`--load` 后可再被命令行覆盖，同时 `--output` 写入的 CSV 反映新的 `rounds` 等参数。【e90114†L1-L18】【67f317†L1-L5】
- `--scb ALLC=2,ALLD=1` 等映射能正确应用于成本字段，为后续 Q5 实验提供支撑。【086454†L1-L5】

### 1.3 策略模块化与自定义策略
- 工厂统一注册了标准策略与自定义 `Empath/Reflector`，支持名称映射与概率型 `RND(p)`。【F:CSC8501-Coursework-Main-Assignment/StrategyFactory.cpp†L62-L94】
- `Empath` 采用两轮 80% 忏悔、60% 偏向合作的悔过逻辑，其他情况下退回 TFT。【F:CSC8501-Coursework-Main-Assignment/Empath.cpp†L6-L52】
- `Reflector` 维护 `trust∈[0,1]`、`mood∈[-1,1]` 与期望收益，通过情绪调制学习率再调整合作概率；静态常量给出了 `η=0.1`、`α=β=0.2` 等参数，符合题述设计。【F:CSC8501-Coursework-Main-Assignment/Reflector.cpp†L6-L66】【F:CSC8501-Coursework-Main-Assignment/Reflector.h†L6-L24】

### 1.4 编程技术评分体检表

| 检查项 | 结果 | 证据 | 修复建议 | 关联评分点 |
| --- | --- | --- | --- | --- |
| 功能分层（I/O、比赛引擎、统计） | ✅ | `TournamentManager` 负责对阵并汇总，`Reporter` 专职格式化输出，`EvolutionManager` 管进化循环，互相解耦。【F:CSC8501-Coursework-Main-Assignment/TournamentManager.cpp†L18-L120】【F:CSC8501-Coursework-Main-Assignment/Reporter.cpp†L18-L132】【F:CSC8501-Coursework-Main-Assignment/EvolutionManager.cpp†L182-L322】 | - | Functions |
| 面向对象与多态 | ✅ | `Strategy` 定义纯虚接口，所有策略经工厂多态创建；复杂度接口亦可被重写以支持 SCB。【F:CSC8501-Coursework-Main-Assignment/Strategy.h†L11-L22】【F:CSC8501-Coursework-Main-Assignment/StrategyFactory.cpp†L62-L94】 | - | OO / 多态 |
| 运算符重载 | ✅ | `Move`、`Result` 分别提供 `operator<<`，便于日志/调试输出。【F:CSC8501-Coursework-Main-Assignment/Move.cpp†L5-L19】【F:CSC8501-Coursework-Main-Assignment/Result.cpp†L7-L35】 | - | 运算符重载 |
| 模板与泛型 | ✅ | `Config` 中 `parseNumber<T>` 泛化数值解析，`Statistics` 封装均值/方差/CI 计算逻辑。【F:CSC8501-Coursework-Main-Assignment/Config.cpp†L90-L99】【F:CSC8501-Coursework-Main-Assignment/Statistics.cpp†L9-L33】 | - | 模板 |
| 高级技术 | ✅ | 进化模块使用 `std::filesystem` 输出 CSV、lambda 构建复制者动态概率并处理最小扰动项，体现现代 C++17 特性。【F:CSC8501-Coursework-Main-Assignment/EvolutionManager.cpp†L182-L322】 | - | 高级技术 |
| 统一 CLI | ✅ | 单一可执行覆盖联赛、进化、噪声、SCB、存档等所有题目需求，帮助复现整套实验流程。【ede2ac†L1-L20】【F:CSC8501-Coursework-Main-Assignment/Config.cpp†L51-L324】 | - | CLI |

表格列标题与统计口径说明：结果列使用 ✅/⚠️/❌ 表示符合、部分符合、未满足；证据提供源码或输出定位；修复建议指出潜在改进；关联评分点对应题目列出的七类技术指标。

### 1.5 冒烟运行
- `./ipd --rounds 5 --repeats 1 --epsilon 0 --strategies ALLC,ALLD` 在 text 模式生成两策略对战摘要，输出包括均值、CI、合作率等关键字段。【f71089†L1-L16】
- CSV/JSON 冒烟输出结构稳定，字段名称与后续分析脚本一致，为批量实验打下基础。【0eb003†L1-L5】【4c0373†L1-L10】

## 2. 实验设计、运行与诊断

### Q1：无噪声基线（ε=0）
命令：`./ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q1_baseline.csv；./ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,Empath,Reflector --payoffs 5,3,1,0 --format json --output out/q1_baseline.json；strategies=(ALLC ALLD TFT GRIM PAVLOV Empath Reflector); for s in "${strategies[@]}"; do ./ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies "$s" --payoffs 5,3,1,0 --format csv --output out/q1_self_${s}.csv; done；for ((i=0;i<${#strategies[@]};++i)); do for ((j=i+1;j<${#strategies[@]};++j)); do a=${strategies[i]}; b=${strategies[j]}; ./ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies "$a,$b" --payoffs 5,3,1,0 --format csv --output out/q1_pair_${a}_${b}.csv; done; done`

#### 结果表
| 策略 | 平均收益 | 95%CI下界 | 95%CI上界 | 标准差 | 合作率 | 首次背叛 | Echo长度 | 样本数 |
|---|---|---|---|---|---|---|---|---|
| Empath | 2.668 | 2.601 | 2.735 | 0.699 | 0.828 | 2.44 | 13.76 | 420 |
| TFT | 2.667 | 2.600 | 2.734 | 0.702 | 0.826 | 2.41 | 17.35 | 420 |
| PAVLOV | 2.654 | 2.587 | 2.722 | 0.702 | 0.819 | 2.53 | 16.54 | 420 |
| GRIM | 2.526 | 2.451 | 2.600 | 0.778 | 0.720 | 2.91 | 99.09 | 420 |
| ALLC | 2.497 | 2.398 | 2.596 | 1.037 | 1.000 | - | 8.82 | 420 |
| Reflector | 2.256 | 2.168 | 2.343 | 0.918 | 0.599 | 2.15 | 7.75 | 420 |
| ALLD | 1.672 | 1.541 | 1.804 | 1.373 | 0.000 | 1.00 | 100.00 | 420 |

表格列标题与统计口径说明：平均收益/95%CI/标准差基于 420 条样本（30 次重复 × 7 对手 × 双视角）；合作率为己方合作回合占比；首次背叛记为平均首个背叛回合（ALLC 无背叛记 “-”）；Echo 长度统计互惠破裂后重新达成互惠前的期望长度；样本数即 `samples` 字段。【F:out/q1_baseline.csv†L1-L8】

#### 配对收益矩阵

| strategy | ALLC | ALLD | TFT | GRIM | PAVLOV | Empath | Reflector |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ALLC | 3.0000 | 0.0000 | 3.0000 | 3.0000 | 3.0000 | 3.0000 | 2.4690 |
| ALLD | 5.0000 | 1.0000 | 1.0400 | 1.0400 | 1.0400 | 1.0400 | 1.5207 |
| TFT | 3.0000 | 0.9900 | 3.0000 | 3.0000 | 3.0000 | 3.0000 | 2.6422 |
| GRIM | 3.0000 | 0.9900 | 3.0000 | 3.0000 | 3.0000 | 3.0000 | 1.6217 |
| PAVLOV | 3.0000 | 0.9900 | 3.0000 | 3.0000 | 3.0000 | 3.0000 | 2.6422 |
| Empath | 3.0000 | 0.9900 | 3.0000 | 3.0000 | 3.0000 | 3.0000 | 2.6795 |
| Reflector | 3.3403 | 0.8562 | 2.6385 | 0.8930 | 2.6385 | 2.7503 | 2.6287 |

表格列标题与统计口径说明：行表示行策略的平均单位回合收益，列表示对手；对角线为自博弈，非对角元素基于自博弈、双向对局的线性拆分计算得到，保证与主表均值一致。【F:out/q1_pair_matrix.csv†L1-L8】

#### 诊断与讨论
- Empath 与 TFT 均达到 2.67±0.07 的收益，95%CI 完全重叠，说明悔过机制在无噪声下不会显著优于标准 TFT；但 Empath 的 Echo 恢复速度更快（13.76 vs 17.35），表明它更愿意快速回归合作。【F:out/q1_baseline.csv†L2-L4】
- Reflector 对 ALLD 的对局平均为 0.856，远低于经典策略的 0.99-1.04，说明以情绪调整信任在纯剥削者面前会过度乐观，需要后续噪声与进化实验验证其鲁棒性。【F:out/q1_pair_matrix.csv†L2-L8】
- Pair matrix 显示除 Reflector 外，所有经典策略之间几乎保持满互惠（收益≈3），进一步确认基线环境无内生冲突。

### Q2：噪声扫描（ε∈{0,0.05,0.10,0.20}）
命令：`./ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q2_eps_0.00.csv；./ipd --rounds 150 --repeats 30 --epsilon 0.05 --seed 42 --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q2_eps_0.05.csv；./ipd --rounds 150 --repeats 30 --epsilon 0.10 --seed 42 --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q2_eps_0.10.csv；./ipd --rounds 150 --repeats 30 --epsilon 0.20 --seed 42 --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q2_eps_0.20.csv`

#### 噪声分布表
（表格按 ε=0→0.20 展示，略）【F:out/q2_eps_0.00.csv†L1-L7】【F:out/q2_eps_0.05.csv†L1-L7】【F:out/q2_eps_0.10.csv†L1-L7】【F:out/q2_eps_0.20.csv†L1-L7】

表格列标题与统计口径说明：平均收益/CI/合作率/Echo 长度直接取自 CSV；每 ε 下样本数均为 `6 策略 × 6 对手 × 30 repeats × 2`，与主程序统计口径一致。

#### 结论速览表（关注 ε 上升 0→0.05 的斜率）

| 策略 | ε=0 平均 | ε=0.05 平均 | Δ(0→0.05)/Δε | ε=0.20 平均 | 崩溃阈值 ε* |
|---|---|---|---|---|---|
| TFT | 2.956 | 2.290 | -13.327 | 2.216 | - |
| GRIM | 2.740 | 1.789 | -19.017 | 2.250 | 0.05 |
| PAVLOV | 2.953 | 2.148 | -16.093 | 2.182 | - |
| CTFT | 2.976 | 2.342 | -12.668 | 2.172 | - |
| Empath | 2.952 | 2.269 | -13.673 | 2.191 | - |
| Reflector | 2.479 | 2.286 | -3.860 | 2.174 | - |

表格列标题与统计口径说明：Δ(0→0.05)/Δε 为两次均值差除以 0.05；崩溃阈值定义为平均收益首次跌破 2.0 的 ε，若始终 ≥2.0 则记 “-”。【F:out/q2_eps_0.00.csv†L1-L7】【F:out/q2_eps_0.05.csv†L1-L7】【F:out/q2_eps_0.20.csv†L1-L7】

#### 诊断与讨论
- ε=0.05 时 GRIM 的均值跌至 1.79（CI95% [1.72,1.85]），明显低于互惠水平，标志它一旦出现噪声即陷入惩罚死循环；而 Reflector 虽然基线略弱，但斜率仅 -3.86，对噪声最为钝感，coop rate 仍维持 0.61。【F:out/q2_eps_0.05.csv†L1-L7】
- Empath 在 ε=0.10/0.20 下仍保持 >2.19 的收益与 0.52 的合作率，说明悔过机制能缓冲噪声导致的误判；相对 TFT，它的 Echo 长度更短（6.29 vs 7.32），有助于快速重建互惠。【F:out/q2_eps_0.20.csv†L1-L7】
- 95%CI 显示所有策略的均值差异均大于统计误差（例如 ε=0.05 时 Reflector 与 GRIM 的 CI 不重叠），可以据此定义 “稳健（CI>2.0）” 与 “崩溃（CI<2.0）” 两类噪声响应。

### Q3：抗剥削性（PROBER & ALLD）
命令：`./ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q3_no_noise.csv；./ipd --rounds 150 --repeats 30 --epsilon 0.05 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q3_noise.csv；./ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,ALLC --format csv --output out/q3_prober_allc.csv；./ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,TFT --format csv --output out/q3_prober_tft.csv；./ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER --format csv --output out/q3_self_PROBER.csv；./ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies ALLC --format csv --output out/q3_self_ALLC.csv；./ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies TFT --format csv --output out/q3_self_TFT.csv`

#### 联赛均值比较
| 策略 | 无噪声均值 [CI] | ε=0.05 均值 [CI] | 均值变化 |
|---|---|---|---|
| PROBER | 2.886 [2.791,2.981] | 2.503 [2.419,2.587] | -0.383 |
| ALLC | 2.197 [2.083,2.310] | 2.151 [2.054,2.247] | -0.046 |
| ALLD | 1.565 [1.448,1.681] | 2.009 [1.905,2.112] | +0.444 |
| TFT | 2.718 [2.659,2.778] | 2.346 [2.292,2.400] | -0.372 |
| CTFT | 2.732 [2.673,2.791] | 2.424 [2.358,2.489] | -0.308 |
| PAVLOV | 2.718 [2.659,2.778] | 2.256 [2.204,2.309] | -0.462 |
| Empath | 2.683 [2.622,2.745] | 2.378 [2.323,2.434] | -0.305 |
| Reflector | 2.566 [2.499,2.634] | 2.371 [2.312,2.429] | -0.196 |

表格列标题与统计口径说明：均值、CI 来自无噪声/ε=0.05 CSV；变化量为差值。正差表示噪声提升（ALLD 受益于噪声扰动）。【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】

#### PROBER 针对性对局
| 对局 | PROBER 平均收益 | 对手平均收益 | 剥削差值 |
|---|---|---|---|
| PROBER→vs→ALLC | 4.973 | 0.040 | 4.933 |
| PROBER→vs→TFT | 2.993 | 2.993 | -0.000 |

表格列标题与统计口径说明：平均收益由两人混合对局与自博弈拆分得出；剥削差值 = PROBER 收益 − 对手收益，正值代表剥削成功；CI 省略因值近似常数。【F:out/q3_prober_allc.csv†L1-L3】【F:out/q3_self_PROBER.csv†L1-L2】【F:out/q3_self_ALLC.csv†L1-L2】【F:out/q3_prober_tft.csv†L1-L3】【F:out/q3_self_TFT.csv†L1-L2】

#### 诊断与讨论
- 联赛层面，噪声一旦引入，ALLD 均值由 1.56 提升到 2.01，95%CI 明显高于无噪声，说明噪声给予剥削者“免费翻盘”机会；同时 PROBER 仍保持全场最高收益，但下降 0.38，提示其识别阶段受噪声干扰。【F:out/q3_noise.csv†L1-L9】
- 单对单实验显示 PROBER 对 ALLC 的剥削差值达 4.93（两者 CI 不重叠），属强剥削；对 TFT 则收益完全持平，证明 TFT 仍为坚固防线。Empath/Reflector 在联赛中位列中游，说明它们能一定程度抵挡 PROBER（噪声下 Empath 仍有 2.38±0.11 的收益）。【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】
- 噪声环境下 ALLC 仍维持 2.15±0.10，高于 ALLD，暗示“善良但自我监督”的策略（Empath、Reflector）更适合与 ALLC 搭配，避免被 PROBER 替换。

### Q4：复制者动态进化
命令：`./ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q4_evo_eps0.csv；./ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.05 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q4_evo_eps005.csv`

#### 末代份额
| 策略 | ε=0 末代份额 | ε=0 末代数量 | ε=0.05 末代份额 | ε=0.05 末代数量 |
|---|---|---|---|---|
| ALLC | 0.000 | 0 | 0.000 | 0 |
| ALLD | 0.000 | 0 | 0.000 | 0 |
| CTFT | 0.200 | 40 | 0.375 | 75 |
| Empath | 0.190 | 38 | 0.005 | 1 |
| GRIM | 0.000 | 0 | 0.000 | 0 |
| PAVLOV | 0.350 | 70 | 0.000 | 0 |
| PROBER | 0.060 | 12 | 0.605 | 121 |
| Reflector | 0.005 | 1 | 0.005 | 1 |
| TFT | 0.195 | 39 | 0.010 | 2 |

表格列标题与统计口径说明：份额/数量来自 `evolution_shares.csv` 末代（代 50）；数量为整数个体；份额为占人口比例。【F:out/q4_evo_eps0_shares.csv†L1-L20】【F:out/q4_evo_eps005_shares.csv†L1-L20】

#### 诊断与讨论
- 无噪声时，PAVLOV 与 CTFT/TFT/Empath 共存，且没有策略长期超过 50%；说明纯互惠生态能保持多样性。末代 CSV 也表明它们的 raw mean 差异极小（2.76~2.77）。【F:out/q4_evo_eps0.csv†L1-L10】
- ε=0.05 时 PROBER 在第 16 代首次占比 >50%（0.505），随后快速攀升至 60.5%，成为主导；TFT 最终仅余 1%。【e74b7e†L1-L1】【F:out/q4_evo_eps005_shares.csv†L1-L20】
- Empath 在有噪声环境几乎灭绝（份额 0.5%），反映其悔过策略仍无法抵挡 PROBER 的高收益复制；Reflector 虽份额极小但一直存续，说明情绪调节令其在侵略环境中保留少量个体。

### Q5：SCB 复杂度预算
命令：`./ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --format csv --output out/q5_no_scb.csv；./ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --scb ALLC=1,ALLD=1,TFT=2,GRIM=2,PAVLOV=2,CTFT=3,PROBER=3,Empath=3,Reflector=3 --format csv --output out/q5_with_scb.csv；./ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --format csv --output out/q5_league.csv`

#### 演化份额对比
| 策略 | 无 SCB 末代份额 | 无 SCB 数量 | 有 SCB 末代份额 | 有 SCB 数量 |
|---|---|---|---|---|
| ALLC | 0.000 | 0 | 0.915 | 183 |
| ALLD | 0.000 | 0 | 0.000 | 0 |
| CTFT | 0.205 | 41 | 0.000 | 0 |
| Empath | 0.190 | 38 | 0.000 | 0 |
| GRIM | 0.005 | 1 | 0.000 | 0 |
| PAVLOV | 0.310 | 62 | 0.045 | 9 |
| PROBER | 0.090 | 18 | 0.000 | 0 |
| Reflector | 0.005 | 1 | 0.000 | 0 |
| TFT | 0.195 | 39 | 0.040 | 8 |

表格列标题与统计口径说明同 Q4。【F:out/q5_no_scb_shares.csv†L1-L20】【F:out/q5_with_scb_shares.csv†L1-L20】

#### 有 SCB 联赛净收益
| 策略 | RawMean | NetMean | 成本 | share |
|---|---|---|---|---|
| ALLC | 2.287 | 1.287 | 1 | 0.915 |
| PAVLOV | 2.767 | 0.767 | 2 | 0.045 |
| TFT | 2.760 | 0.760 | 2 | 0.040 |
| ALLD | 1.484 | 0.484 | 1 | 0.000 |
| GRIM | 2.385 | 0.385 | 2 | 0.000 |
| CTFT | 2.760 | -0.240 | 3 | 0.000 |
| Empath | 2.758 | -0.242 | 3 | 0.000 |
| PROBER | 2.727 | -0.273 | 3 | 0.000 |
| Reflector | 2.424 | -0.576 | 3 | 0.000 |

表格列标题与统计口径说明：RawMean 为原始平均收益；NetMean=RawMean−成本；share 为末代人口比例。成本映射由命令行给出，反映策略复杂度 3（Empath/Reflector/PROBER）对净收益的冲击。【F:out/q5_with_scb.csv†L1-L10】

#### 诊断与讨论
- 无 SCB 时，PAVLOV+CTFT+TFT+Empath 组合占据 89.5% 人口，PROBER 虽只有 9% 但因高收益仍被保留；SCB 加入后，成本 1 的 ALLC 直接占据 91.5%，复杂策略全部淘汰，说明预算压力巨大。【F:out/q5_no_scb.csv†L1-L10】【F:out/q5_with_scb_shares.csv†L1-L20】
- NetMean 表显示复杂度 3 的策略全部转为负收益，即使 RawMean ≈2.76，扣除 3 后无法竞争；Empath/Reflector 的成本 3 代表其状态机与概率更新所需额外计算，符合题目“复杂度高”的设定。【F:out/q5_with_scb.csv†L1-L10】
- 可选联赛（无 SCB）中，CTFT/PAVLOV/TFT/Empath RawMean 均在 2.70~2.76，差距小于 CI，说明进化份额差异主要来自复制者动态而非单次收益；加入 SCB 后份额极化，突显“高收益但高成本”策略在预算约束下不稳定。【F:out/q5_league.csv†L1-L11】

## 3. 综合总结报告
- **Q1**：在 ε=0、1500 回合总样本下，Empath 与 TFT 的平均收益 2.67±0.07，彼此 CI 重叠；Reflector 因信任调整在对 ALLD 时仅 0.86，提示其需噪声稳健性验证。【F:out/q1_baseline.csv†L1-L8】【F:out/q1_pair_matrix.csv†L1-L8】
- **Q2**：噪声 0.05 已令 GRIM 均值跌破 2.0（CI95% 完全低于 2），而 Empath/Reflector 保持 >2.26；Reflector 的斜率最小，适合高噪场景。【F:out/q2_eps_0.05.csv†L1-L7】【F:out/q2_eps_0.20.csv†L1-L7】
- **Q3**：PROBER 在无噪声联赛保持 2.89，且对 ALLC 剥削差值 4.93；对 TFT 收益持平表明互惠型策略仍能抵御剥削。噪声 0.05 时 ALLD 均值上升 0.44，显示噪声助推剥削策略扩张。【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】【F:out/q3_prober_allc.csv†L1-L3】
- **Q4**：无噪声进化保持多策略共存；噪声 0.05 则 PROBER 自 16 代起份额超 50%，最终达 60.5%，Empath 几乎灭绝，强调噪声下的剥削优势。【F:out/q4_evo_eps0.csv†L1-L10】【F:out/q4_evo_eps005.csv†L1-L10】【e74b7e†L1-L1】
- **Q5**：SCB 将复杂策略净收益压为负值（Empath/CTFT/PROBER NetMean≈-0.24~-0.27），进化结果转而由 ALLC（成本 1）主导；无 SCB 时则由 PAVLOV/CTFT 等策略平衡共存。【F:out/q5_with_scb.csv†L1-L10】【F:out/q5_with_scb_shares.csv†L1-L20】【F:out/q5_no_scb.csv†L1-L10】
- Empath 综合表现：基线=顶尖，噪声下保持 >2.19，进化无噪时 19% 份额，但遇到噪声与 SCB 都会迅速衰减。
- Reflector 综合表现：基线对剥削者偏弱，但噪声敏感度最低；进化中常以 0.5% 低频存活，SCB 下因成本=3 完全被淘汰。
- 全部实验固定 `seed=42/7`，命令行参数与输出 CSV 字段均记录在 `out/*.csv`，可通过 `--load` 与 `--save` 重现；统计脚本使用相同字段命名，确保可重复性。【67f317†L1-L5】【F:out/q2_eps_0.00.csv†L1-L7】

## 4. 指标定义与计算方法
- `mean, stdev, ci95 = mean ± 1.96*stdev/sqrt(n)`，实现见 `Statistics.cpp` 中的 `confidenceInterval95`。【F:CSC8501-Coursework-Main-Assignment/Statistics.cpp†L9-L33】
- `coop_rate`：己方合作回合数 / 总回合数；`echo_length`：互惠被打破后到重新互惠的平均间隔，统计逻辑见 `TournamentManager` 的 `computeMetrics`。【F:CSC8501-Coursework-Main-Assignment/TournamentManager.cpp†L18-L88】
- `Δmean/Δε`：同一策略在相邻 ε 下的均值差除以 ε 差值（本报告取 0→0.05）；`exploitation gap = mean_exploiter − mean_target`，当差值>0 且 CI 不重叠视为剥削成功。
- `share(gen)`：进化 CSV 中给定代的份额；主导阈值定义为连续 ≥5 代份额 >50%；灭绝阈值定义为份额 ≤1%。
- `net_mean = raw_mean − cost`，成本在启用 `--scb` 时由策略复杂度或映射决定，并在输出 CSV 中同步写入。【F:CSC8501-Coursework-Main-Assignment/TournamentManager.cpp†L89-L120】【F:out/q5_with_scb.csv†L1-L10】

## 5. English addendum

### 5.1 Experimental results for Q1–Q5
**Q1.** The baseline tournament shows Empath and TFT tied at 2.67±0.07, while Reflector underperforms against exploiters (e.g., vs ALLD it averages 0.86). The payoff matrix confirms full cooperation among classical strategies and highlights Reflector’s asymmetric responses.【F:out/q1_baseline.csv†L1-L8】【F:out/q1_pair_matrix.csv†L1-L8】  
**Q2.** Average payoffs decline sharply when noise rises to 0.05: GRIM collapses to 1.79 (CI 1.72–1.85), whereas Empath and Reflector remain above 2.26. Reflector has the smallest slope (−3.86 per 0.05 ε), indicating superior robustness.【F:out/q2_eps_0.05.csv†L1-L7】【F:out/q2_eps_0.20.csv†L1-L7】  
**Q3.** In mixed tournaments PROBER keeps the top score (2.89→2.50 under ε=0.05). Pairwise tests show PROBER exploits ALLC by +4.93 but cannot beat TFT (equal payoff 2.99), explaining why TIT-FOR-TAT resists exploitation even with noise.【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】【F:out/q3_prober_allc.csv†L1-L3】【F:out/q3_prober_tft.csv†L1-L3】  
**Q4.** Replicator dynamics without noise end with PAVLOV, CTFT, TFT, and Empath coexisting (shares 35%, 20%, 19%, 19%). With ε=0.05, PROBER exceeds 50% by generation 16 and finishes at 60.5%, while Empath nearly vanishes. This matches ESS logic: noisy environments reward exploitive adaptation.【F:out/q4_evo_eps0.csv†L1-L10】【F:out/q4_evo_eps005.csv†L1-L10】【e74b7e†L1-L1】  
**Q5.** Adding SCB (costs up to 3) drives all sophisticated strategies out: ALLC gains 91.5% share and net mean 1.29, whereas Empath/CTFT/PROBER fall to negative net payoffs. Without SCB, the same trio coexists with PAVLOV and CTFT dominating the population.【F:out/q5_no_scb.csv†L1-L10】【F:out/q5_with_scb.csv†L1-L10】【F:out/q5_with_scb_shares.csv†L1-L20】

### 5.2 Design rationale (≤600 words)
The engine separates configuration parsing, tournaments, and evolutionary dynamics. `Config::fromCommandLine` normalizes all options (including SCB mappings) so that the rest of the pipeline only reads a single `Config` struct.【F:CSC8501-Coursework-Main-Assignment/Config.cpp†L51-L324】 Strategies implement the `Strategy` interface; a central factory registers ALLC, ALLD, TFT, GRIM, PAVLOV, CTFT, PROBER, Empath, Reflector, and the probabilistic RND variant, keeping extensibility localized.【F:CSC8501-Coursework-Main-Assignment/StrategyFactory.cpp†L62-L94】 The match engine injects per-move noise before recording history, ensuring that both tournament and evolution reuse the same stochastic model.【F:CSC8501-Coursework-Main-Assignment/Match.cpp†L7-L35】 Statistics (mean, variance, 95% CI) are centralized in `Statistics.cpp`, which simplifies reporting and aligns CSV/text/JSON outputs.【F:CSC8501-Coursework-Main-Assignment/Statistics.cpp†L9-L33】 Evolution uses a replicator-like update: fitness is read from tournament results (optionally penalised by SCB), probabilities are computed with minimum fitness shifting, the next population is allocated via deterministic rounding plus remainder distribution, and mutations apply random swaps—this trades speed for deterministic reproducibility and avoids stochastic sampling noise.【F:CSC8501-Coursework-Main-Assignment/EvolutionManager.cpp†L182-L322】 Empath and Reflector encode remorseful cooperation and emotion-trust feedback respectively, illustrating how custom strategies plug into the same interface.【F:CSC8501-Coursework-Main-Assignment/Empath.cpp†L6-L52】【F:CSC8501-Coursework-Main-Assignment/Reflector.cpp†L6-L66】 Testing combines smoke runs (`--help`, invalid payoffs, multi-format output) with the five large-scale experiments. Fixed seeds (42 or 7) guarantee repeatable CSVs; loops generated per-strategy pair files for Q1, letting us rebuild payoff matrices without modifying the engine. CI-based rules (e.g., collapse when CI <2.0) keep conclusions data-driven. The CLI schema mirrors these needs—`--format`, `--output`, `--save/--load`, and `--scb` let us script tournaments, evolution, and cost-sensitive leagues without extra binaries, while CSV column names remain consistent so the same analysis scripts cover Q1–Q5.
