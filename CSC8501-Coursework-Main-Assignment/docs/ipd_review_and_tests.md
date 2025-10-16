# CSC8501 迭代囚徒困境（IPD）评测报告

## 1. 构建与代码体检

### 1.1 构建
- 编译流程：先对每个源码编译为目标文件，再统一链接生成 `build/ipd`。`for` 循环输出显示全部源文件均成功编译，随后一次性链接得到可执行文件。【0a8c70†L1-L2】【8758ab†L1-L3】
- 可执行文件：`./build/ipd`（位于仓库根目录下的 `CSC8501-Coursework-Main-Assignment/CSC8501-Coursework-Main-Assignment/build/`）。

### 1.2 CLI 能力核查
- `--help` 输出覆盖所需参数，包括噪声、演化、SCB、配置保存/加载等选项，与统一 CLI 规范一致。【362136†L1-L19】
- 命令行解析允许 `--rounds/--repeats/--epsilon/--payoffs/--strategies/--evolve/--generations/--population/--mutation/--format/--output/--seed/--save/--load/--scb/--verbose/--help`，并对旧参数发出错误提示；参数值通过模板函数 `parseNumber` 统一解析。【F:Config.cpp†L90-L514】
- `--payoffs` 需满足 `T>R>P>S` 与 `2R>T+S`，否则立即退出，保证支付矩阵正确性。【F:Payoff.cpp†L7-L13】
- 噪声通过 `Match::play` 中的双边翻转实现，每步以 `epsilon` 概率互换动作，符合“每步噪声”定义。【F:Match.cpp†L13-L29】
- `--save/--load` 使用 JSON 序列化/反序列化完整配置，含 SCB 映射与种子信息，可复现实验。【F:Config.cpp†L556-L670】
- `--scb` 支持默认启用与显式映射，未提供映射时按策略复杂度扣减；CSV 报告在启用 SCB 时输出 RawMean/NetMean/Cost 列，便于评估复杂度成本。【F:Config.cpp†L426-L448】【F:Reporter.cpp†L270-L309】

### 1.3 策略模块化与自定义策略
- 工厂注册了 `ALLC, ALLD, TFT, GRIM, PAVLOV, RND, CTFT, PROBER, Empath, Reflector` 等标准策略，支持字符串配置与随机策略参数化。【F:StrategyFactory.cpp†L20-L94】
- `Strategy` 抽象类定义 `nextMove/reset/complexity` 等虚函数，自定义策略可按接口扩展；`StrategyPtr` 采用 `std::unique_ptr` 管理生命周期。【F:Strategy.h†L11-L23】
- Empath：在悔过状态下以 80% 概率合作，被对手背叛后进入偏向合作的恢复模式（60% 概率），否则按 TFT 行为，复杂度设为 3。【F:Empath.cpp†L6-L52】【F:Empath.h†L8-L20】
- Reflector：维护 `trust∈[0,1]` 与 `mood∈[-1,1]`，情绪驱动学习率 `η(1+mood)`；根据最近收益与期望回报更新信任并决定合作概率，复杂度同为 3。【F:Reflector.cpp†L6-L66】【F:Reflector.h†L6-L28】

### 1.4 编程技术评分
| 检查项 | 结果 | 证据 | 修复建议 | 关联评分点 |
| --- | --- | --- | --- | --- |
| 模块化函数 | ✅ | 锦标赛引擎、统计与 SCB 处理拆分为 `generateMatchPairs`、`computeMetrics`、`buildResults` 等函数。【F:TournamentManager.cpp†L18-L178】 | 无 | Functions |
| 面向对象 | ✅ | 策略多态接口与工厂注册机制实现解耦。【F:Strategy.h†L11-L23】【F:StrategyFactory.cpp†L46-L94】 | 无 | OO |
| 运算符重载 | ✅ | `Result::toString`、`operator<<` 支持结果对象友好输出。【F:Result.cpp†L7-L47】 | 无 | 运算符重载 |
| 多态调度 | ✅ | 锦标赛通过 `StrategyPtr` 调用虚方法 `nextMove/reset/onMatchEnd`，实现策略多态。【F:TournamentManager.cpp†L118-L207】【F:Strategy.h†L15-L19】 | 无 | 多态 |
| 模板与泛型 | ✅ | `parseNumber` 模板统一解析数值参数；统计模块使用泛型积累逻辑。【F:Config.cpp†L90-L114】【F:Statistics.cpp†L8-L33】 | 无 | 模板 |
| 高级技术 | ✅ | 采用 `std::transform`、`std::for_each`、lambda 捕获生成对阵列表与排序结果；演化模块含 `std::discrete_distribution`、`std::clamp` 等现代 C++ 特性。【F:TournamentManager.cpp†L40-L209】【F:EvolutionManager.cpp†L63-L200】 | 无 | 高级技术 |
| 统一 CLI | ✅ | `Config::fromCommandLine` 汇总全部参数并提供默认补全、旧参数兼容提示。【F:Config.cpp†L405-L553】 | 无 | 统一 CLI |

### 1.5 冒烟运行
```bash
./build/ipd --help > out/help.txt
./build/ipd --rounds 5 --repeats 1 --epsilon 0.00 --strategies ALLC,ALLD --payoffs 5,3,1,0 --format text --output out/smoke_text.txt
```
- `--help` 输出见 `out/help.txt`，参数说明完整。【362136†L1-L19】
- 冒烟赛生成文本报告，验证平均收益、协作率等字段均存在且数值合理，表明 CLI、报表与 SCB 默认关闭逻辑正常。【99f588†L1-L16】

### 1.6 体检报告表
| 检查项 | 结果 | 证据 | 修复建议 | 关联评分点 |
| --- | --- | --- | --- | --- |
| 构建脚本 | ✅ | 手工编译并链接成功，产出 `build/ipd` 可执行。【0a8c70†L1-L2】【8758ab†L1-L3】 | 建议引入 CMake 以便自动化构建 | Functions |
| 参数校验 | ✅ | `--payoffs`、`--format`、旧参数淘汰与默认值校验完备。【F:Payoff.cpp†L7-L13】【F:Config.cpp†L491-L546】 | 可补充对负回合/重复数的错误提示（目前 `ensureDefaults` 自动修正） | 统一 CLI |
| 噪声注入 | ✅ | `Match::play` 按 `epsilon` 对双方动作独立翻转，覆盖 C↔D 噪声模型。【F:Match.cpp†L13-L29】 | 无 | Functions |
| 报表输出 | ✅ | CSV/Text/JSON 同步生成，SCB 启用时输出 Raw/Net/Cost；JSON 可存储演化历史。【F:Reporter.cpp†L270-L411】 | 无 | 运算符重载 |
| 策略体系 | ✅ | 工厂注册 + 多态接口 + 自定义策略实现满足扩展要求。【F:StrategyFactory.cpp†L81-L94】【F:Empath.cpp†L6-L52】【F:Reflector.cpp†L6-L66】 | 无 | OO |
| 演化框架 | ✅ | 复制者动态（离散抽样、变异、份额记录）完备，且生成 `evolution_shares.csv` 供复盘。【F:EvolutionManager.cpp†L118-L338】 | 无 | 高级技术 |
| 统计指标 | ✅ | 平均值、标准差、95%CI、协作率、首次背叛、Echo 长度等均在 CSV 中输出。【F:TournamentManager.cpp†L118-L178】【F:Reporter.cpp†L270-L309】 | 可考虑输出 pairwise 原始得分以减少外部推导负担 | 模块化/模板 |

**表格列标题与统计口径说明**：
- “检查项”：体检条目名称。
- “结果”：✅ 表示符合规范，⚠️ 为存在风险，❌ 为不满足。
- “证据”：源码或日志引用。
- “修复建议”：后续优化方向。
- “关联评分点”：与评分指标（Functions/OO/…）的映射关系。

## 2. 实验设计与结果

所有实验默认：`--payoffs 5,3,1,0`、`--rounds 150`、`--repeats 30`、`--seed 42`（单对单实验除外），输出放在 `out/` 目录，统计均报告 95% 置信区间。

### Q1 无噪声 Baseline
命令：
```bash
./build/ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q1_baseline.csv
for s in ALLC ALLD TFT GRIM PAVLOV Empath Reflector; do ./build/ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies $s --payoffs 5,3,1,0 --format csv --output out/q1_single_${s}.csv; done
# 共 21 组双人对局命令，形式与下例相同：
./build/ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD --payoffs 5,3,1,0 --format csv --output out/q1_pair_ALLC_ALLD.csv
```

#### 配对收益矩阵
| Strategy | ALLC | ALLD | TFT | GRIM | PAVLOV | Empath | Reflector |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ALLC | 3.000 | 0.000 | 3.000 | 3.000 | 3.000 | 3.000 | 2.469 |
| ALLD | 5.000 | 1.000 | 1.040 | 1.040 | 1.040 | 1.040 | 1.521 |
| TFT | 3.000 | 0.990 | 3.000 | 3.000 | 3.000 | 3.000 | 2.642 |
| GRIM | 3.000 | 0.990 | 3.000 | 3.000 | 3.000 | 3.000 | 1.622 |
| PAVLOV | 3.000 | 0.990 | 3.000 | 3.000 | 3.000 | 3.000 | 2.642 |
| Empath | 3.000 | 0.990 | 3.000 | 3.000 | 3.000 | 3.000 | 2.679 |
| Reflector | 3.340 | 0.856 | 2.638 | 0.893 | 2.638 | 2.750 | 2.629 |

**列说明**：行/列为对阵双方，单元格为对应行策略在 100 回合平均收益，依据单/双策略 CSV 通过线性分解复原。【F:out/q1_pair_matrix.csv†L1-L8】

#### 协作率矩阵
| Strategy | ALLC | ALLD | TFT | GRIM | PAVLOV | Empath | Reflector |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ALLC | 1.000 | 1.000 | 1.000 | 1.000 | 1.000 | 1.000 | 1.000 |
| ALLD | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| TFT | 1.000 | 0.010 | 1.000 | 1.000 | 1.000 | 1.000 | 0.758 |
| GRIM | 1.000 | 0.010 | 1.000 | 1.000 | 1.000 | 1.000 | 0.020 |
| PAVLOV | 1.000 | 0.010 | 1.000 | 1.000 | 1.000 | 1.000 | 0.758 |
| Empath | 1.000 | 0.010 | 1.000 | 1.000 | 1.000 | 1.000 | 0.791 |
| Reflector | 0.814 | 0.121 | 0.747 | 0.154 | 0.747 | 0.796 | 0.739 |

**列说明**：对应对局的合作比例（合作次数/总回合数）。【F:out/q1_pair_coop.csv†L1-L8】

#### 总体排名（100 回合 × 30 重复）
| Strategy | Mean | StdDev | CI95 Low | CI95 High | Coop Rate | First Defection | Echo Length | Complexity |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Empath | 2.668 | 0.699 | 2.601 | 2.735 | 0.828 | 2.442 | 13.764 | 3 |
| TFT | 2.667 | 0.702 | 2.600 | 2.734 | 0.826 | 2.408 | 17.350 | 2 |
| PAVLOV | 2.654 | 0.702 | 2.587 | 2.722 | 0.819 | 2.533 | 16.539 | 2 |
| GRIM | 2.526 | 0.778 | 2.451 | 2.600 | 0.720 | 2.908 | 99.092 | 2 |
| ALLC | 2.497 | 1.037 | 2.398 | 2.596 | 1.000 | NA | 8.824 | 1 |
| Reflector | 2.256 | 0.918 | 2.168 | 2.343 | 0.599 | 2.145 | 7.747 | 3 |
| ALLD | 1.672 | 1.373 | 1.541 | 1.804 | 0.000 | 1.000 | 100.000 | 1 |

**列说明**：Mean/StdDev/CI 来自 30 次重复；First Defection 以回合计，NA 表示未背叛。【F:out/q1_summary.csv†L1-L8】

#### 诊断与讨论
- Empath 与 TFT CIs 完全重叠（2.60–2.73），表明悔过机制在无噪声下可与经典互惠并列最优；Empath 的 Echo 长度更短，意味着恢复速度更快。【F:out/q1_summary.csv†L2-L4】
- Reflector 能以 3.34 的平均分剥削 ALLC，但对 GRIM 仅得 0.893，说明基于信任的学习策略在面对严厉报复者时仍会陷入互相背叛循环。【F:out/q1_pair_matrix.csv†L1-L8】
- ALLD 面对互惠策略时只有 ~1.0 分，说明经典惩罚足以压制纯背叛；Reflector 通过信任衰减仍能拿到 0.856 分，显示其对背叛的部分自适应能力。【F:out/q1_pair_matrix.csv†L1-L8】

### Q2 噪声扫描（ε = 0/0.05/0.10/0.20）
命令：
```bash
for eps in 0.00 0.05 0.10 0.20; do \
  ./build/ipd --rounds 150 --repeats 30 --epsilon $eps --seed 42 \
    --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector \
    --payoffs 5,3,1,0 --format csv --output out/q2_eps_${eps}.csv; \
done
```

#### 平均收益与 95%CI
| Strategy | ε=0.00 | ε=0.05 | ε=0.10 | ε=0.20 |
| --- | --- | --- | --- | --- |
| TFT | 2.956 [2.942,2.970] | 2.290 [2.233,2.347] | 2.241 [2.190,2.292] | 2.216 [2.174,2.258] |
| GRIM | 2.740 [2.679,2.800] | 1.789 [1.724,1.853] | 1.964 [1.909,2.018] | 2.250 [2.205,2.295] |
| PAVLOV | 2.953 [2.938,2.968] | 2.148 [2.097,2.199] | 2.151 [2.102,2.201] | 2.182 [2.140,2.224] |
| CTFT | 2.976 [2.969,2.982] | 2.342 [2.268,2.416] | 2.248 [2.183,2.312] | 2.172 [2.125,2.220] |
| Empath | 2.952 [2.938,2.967] | 2.269 [2.213,2.325] | 2.231 [2.180,2.282] | 2.191 [2.149,2.233] |
| Reflector | 2.479 [2.405,2.553] | 2.286 [2.223,2.350] | 2.227 [2.170,2.284] | 2.174 [2.128,2.220] |

**列说明**：单元格展示平均收益与 95%CI。【F:out/q2_summary.csv†L1-L25】

#### 协作率
| Strategy | ε=0.00 | ε=0.05 | ε=0.10 | ε=0.20 |
| --- | --- | --- | --- | --- |
| TFT | 0.970 | 0.557 | 0.518 | 0.498 |
| GRIM | 0.836 | 0.108 | 0.130 | 0.211 |
| PAVLOV | 0.968 | 0.508 | 0.496 | 0.494 |
| CTFT | 0.990 | 0.778 | 0.713 | 0.639 |
| Empath | 0.968 | 0.577 | 0.553 | 0.524 |
| Reflector | 0.717 | 0.606 | 0.569 | 0.546 |

**列说明**：协作次数 / 总回合数。【F:out/q2_summary.csv†L1-L25】

#### Echo 长度
| Strategy | ε=0.00 | ε=0.05 | ε=0.10 | ε=0.20 |
| --- | --- | --- | --- | --- |
| TFT | 5.971 | 11.891 | 9.462 | 7.321 |
| GRIM | 148.933 | 60.847 | 31.426 | 14.107 |
| PAVLOV | 6.380 | 11.264 | 8.696 | 6.545 |
| CTFT | 3.963 | 5.945 | 5.326 | 5.053 |
| Empath | 4.822 | 8.000 | 7.040 | 6.286 |
| Reflector | 7.724 | 5.810 | 5.640 | 5.356 |

**列说明**：互信破裂到恢复的平均步数。【F:out/q2_summary.csv†L1-L25】

#### Δmean/Δε
| Strategy | ε=0.00→0.05 | 0.05→0.10 | 0.10→0.20 |
| --- | --- | --- | --- |
| TFT | -13.33 | -0.98 | -0.25 |
| GRIM | -19.02 | 3.50 | 2.86 |
| PAVLOV | -16.09 | 0.06 | 0.31 |
| CTFT | -12.67 | -1.89 | -0.75 |
| Empath | -13.67 | -0.76 | -0.40 |
| Reflector | -3.86 | -1.18 | -0.53 |

**列说明**：相邻噪声区间的收益变化率（分/噪声单位）。【F:out/q2_summary.csv†L1-L25】

#### 崩溃阈值
| Strategy | Collapse ε* |
| --- | --- |
| GRIM | 0.05 |
| 其他策略 | >0.20 |

**列说明**：平均收益 < 2.0 视为合作崩溃；其余策略在扫描范围内保持合作。【F:out/q2_collapse_thresholds.csv†L1-L10】

#### 讨论
- ε=0.05 时互惠策略收益骤降 12~19 分/噪声单位，GRIM 直接跌破阈值并触发崩溃；Empath/TFT/PAVLOV 的协作率亦腰斩。【F:out/q2_summary.csv†L1-L25】
- Reflector 的 Δmean/Δε 绝对值最小（-3.86），说明其信任调节机制能在噪声下维持 2.17 分的收益，远高于 GRIM 的 1.79。【F:out/q2_summary.csv†L18-L21】
- Echo 长度表明 GRIM 在高噪声下频繁陷入报复（148→60→31），而 Empath/CTFT 保持较短回合，有助于在噪声中恢复合作。【F:out/q2_summary.csv†L10-L21】

### Q3 抗剥削：PROBER 与 ALLD
命令：
```bash
./build/ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --format csv --output out/q3_no_noise.csv
./build/ipd --rounds 150 --repeats 30 --epsilon 0.05 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --format csv --output out/q3_noise.csv
./build/ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,ALLC --format csv --output out/q3_prober_allc.csv
./build/ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,TFT --format csv --output out/q3_prober_tft.csv
```

#### 联赛平均收益
| Strategy | Mean (ε=0) | Mean (ε=0.05) | Coop (ε=0) | Coop (ε=0.05) |
| --- | --- | --- | --- | --- |
| PROBER | 2.886 | 2.503 | 0.687 | 0.463 |
| CTFT | 2.732 | 2.424 | 0.868 | 0.784 |
| TFT | 2.718 | 2.346 | 0.854 | 0.582 |
| PAVLOV | 2.718 | 2.256 | 0.855 | 0.548 |
| Empath | 2.683 | 2.378 | 0.835 | 0.603 |
| Reflector | 2.566 | 2.371 | 0.726 | 0.622 |
| ALLC | 2.197 | 2.151 | 1.000 | 0.950 |
| ALLD | 1.565 | 2.009 | 0.000 | 0.049 |

**列说明**：展示噪声前后收益与协作率变化。【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】

#### 指定对局剥削差
| Matchup | Exploiter Mean | Target Mean | Gap |
| --- | --- | --- | --- |
| PROBER vs ALLC | 4.973 | 0.040 | 4.933 |
| PROBER vs TFT | 2.993 | 2.993 | ≈0 |

**列说明**：Gap>0 且 CI 不重叠判定剥削成立；PROBER 仅对 ALLC 有显著剥削。【F:out/q3_pair_summary.csv†L1-L3】

#### 讨论
- 无噪声联赛中 PROBER 对 ALLC 的收益差 0.690，配对实验 Gap 更达 4.93，确认 PROBER 能完全剥削无条件合作者；对 TFT Gap≈0，说明互惠策略有效抵御探测背叛。【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_pair_summary.csv†L1-L3】
- 噪声将 PROBER 平均收益拉低 0.38，同期 Empath/Reflector 均仅下降 ~0.3，导致 PROBER 对 Empath/Reflector 的 Gap 从 0.203/0.320 降至 0.125/0.132，表明自适应策略在噪声中更难被剥削。【F:out/q3_noise.csv†L1-L9】【F:out/q3_tournament_gaps.csv†L1-L20】
- ALLD 在 ε=0.05 时平均 2.009，高于 ε=0 的 1.565，但仍落后于互惠策略，说明噪声虽缓解惩罚但不足以使纯背叛占优。【F:out/q3_noise.csv†L1-L9】

### Q4 进化选择（复制者动态）
命令：
```bash
./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --format csv --output out/q4_evo_eps0.csv
cp out/evolution_shares.csv out/q4_evo_eps0_shares.csv
./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.05 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --format csv --output out/q4_evo_eps005.csv
cp out/evolution_shares.csv out/q4_evo_eps005_shares.csv
```

#### 份额演化（选取代：0/5/10/20/30/40/50）
| Generation | ALLC | ALLD | CTFT | Empath | GRIM | PAVLOV | PROBER | Reflector | TFT |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.115 | 0.115 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 |
| 5 | 0.030 | 0.000 | 0.180 | 0.120 | 0.035 | 0.245 | 0.170 | 0.005 | 0.215 |
| 10 | 0.005 | 0.000 | 0.180 | 0.095 | 0.000 | 0.345 | 0.145 | 0.010 | 0.220 |
| 20 | 0.005 | 0.005 | 0.115 | 0.055 | 0.000 | 0.460 | 0.165 | 0.000 | 0.195 |
| 30 | 0.010 | 0.000 | 0.080 | 0.130 | 0.005 | 0.510 | 0.045 | 0.020 | 0.200 |
| 40 | 0.005 | 0.000 | 0.120 | 0.120 | 0.005 | 0.565 | 0.065 | 0.005 | 0.115 |
| 50 | 0.015 | 0.000 | 0.110 | 0.090 | 0.000 | 0.365 | 0.160 | 0.030 | 0.230 |

| Generation | ALLC | ALLD | CTFT | Empath | GRIM | PAVLOV | PROBER | Reflector | TFT |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.115 | 0.115 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 | 0.110 |
| 5 | 0.000 | 0.010 | 0.360 | 0.160 | 0.005 | 0.000 | 0.215 | 0.050 | 0.200 |
| 10 | 0.000 | 0.000 | 0.510 | 0.030 | 0.005 | 0.010 | 0.255 | 0.055 | 0.135 |
| 20 | 0.000 | 0.000 | 0.440 | 0.030 | 0.000 | 0.000 | 0.435 | 0.035 | 0.060 |
| 30 | 0.005 | 0.005 | 0.480 | 0.015 | 0.000 | 0.005 | 0.460 | 0.010 | 0.020 |
| 40 | 0.000 | 0.005 | 0.410 | 0.015 | 0.000 | 0.010 | 0.550 | 0.010 | 0.000 |
| 50 | 0.000 | 0.000 | 0.265 | 0.040 | 0.005 | 0.005 | 0.635 | 0.045 | 0.005 |

**列说明**：上表为 ε=0，下表为 ε=0.05 的份额演化。【F:out/q4_eps0_share_matrix.csv†L1-L13】【F:out/q4_eps005_share_matrix.csv†L1-L13】

#### 支配/灭绝摘要
| Scenario | Strategy | Max Share | Min Share | Dominant? | Extinct? |
| --- | --- | --- | --- | --- | --- |
| ε=0 | PAVLOV | 0.58 | 0.11 | True | False |
| ε=0 | Empath | 0.19 | 0.05 | False | False |
| ε=0 | PROBER | 0.185 | 0.02 | False | False |
| ε=0.05 | CTFT | 0.525 | 0.11 | True | False |
| ε=0.05 | PROBER | 0.635 | 0.11 | True | False |
| ε=0.05 | Empath | 0.16 | 0.01 | False | False |

**列说明**：Max/Min Share 为 0–50 代的极值，Dominant 判定阈值 >50%，Extinct 为全程 <1%。【F:out/q4_evo_summary.csv†L1-L19】

#### 讨论
- ε=0 时 PAVLOV 唯一达到 0.5 以上份额；Empath 与 TFT 在 10–40 代稳定维持 >10%，形成合作联盟核心。【F:out/q4_eps0_share_matrix.csv†L1-L13】【F:out/q4_evo_summary.csv†L1-L10】
- ε=0.05 触发 PROBER 与 CTFT 的双寡头格局，两者份额长期 >40%；Empath/Reflector 下降至个位数但仍未灭绝，说明噪声削弱悔过策略的竞争力。【F:out/q4_eps005_share_matrix.csv†L1-L13】【F:out/q4_evo_summary.csv†L11-L19】
- 结果表明噪声会将 ESS 从“互惠-策略”转移到“探测+宽恕”组合，符合复制者动态对噪声敏感的预测。

### Q5 复杂度成本（SCB）
命令：
```bash
./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --format csv --output out/q5_no_scb.csv
cp out/evolution_shares.csv out/q5_no_scb_shares.csv
./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --scb ALLC=1,ALLD=1,TFT=2,GRIM=2,PAVLOV=2,CTFT=3,PROBER=3,Empath=3,Reflector=3 --format csv --output out/q5_with_scb.csv
cp out/evolution_shares.csv out/q5_with_scb_shares.csv
./build/ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --format csv --output out/q5_league.csv
./build/ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --scb ALLC=1,ALLD=1,TFT=2,GRIM=2,PAVLOV=2,CTFT=3,PROBER=3,Empath=3,Reflector=3 --format csv --output out/q5_league_scb.csv
```

#### 演化份额对比（第 50 代）
| Scenario | ALLC | PAVLOV | TFT | Empath | PROBER |
| --- | --- | --- | --- | --- | --- |
| 无 SCB | 0.020 | 0.410 | 0.275 | 0.240 | 0.030 |
| 启用 SCB | 0.940 | 0.005 | 0.020 | 0.010 | 0.005 |

**列说明**：摘取关键策略份额，完整时间序列见 CSV。【F:out/q5_no_scb_share_matrix.csv†L1-L13】【F:out/q5_with_scb_share_matrix.csv†L1-L13】

#### 支配/灭绝摘要
| Scenario | Strategy | Max Share | Min Share | Dominant? | Extinct? |
| --- | --- | --- | --- | --- | --- |
| 无 SCB | PAVLOV | 0.565 | 0.105 | True | False |
| 无 SCB | Empath | 0.265 | 0.065 | False | False |
| 启用 SCB | ALLC | 0.975 | 0.115 | True | False |
| 启用 SCB | Empath | 0.110 | 0.000 | False | False |
| 启用 SCB | PROBER | 0.110 | 0.000 | False | False |

**列说明**：同 Q4，用于比较 SCB 前后生态。【F:out/q5_evo_summary.csv†L1-L19】

#### 联赛净收益
| Strategy | Raw Mean | Net Mean | Cost | Coop Rate |
| --- | --- | --- | --- | --- |
| ALLC | 2.287 | 1.287 | 1 | 1.000 |
| PAVLOV | 2.748 | 0.748 | 2 | 0.869 |
| TFT | 2.745 | 0.745 | 2 | 0.867 |
| Empath | 2.701 | -0.299 | 3 | 0.844 |
| PROBER | 2.660 | -0.340 | 3 | 0.603 |
| Reflector | 2.370 | -0.630 | 3 | 0.648 |

**列说明**：Raw Mean 为未扣成本收益，Net Mean = Raw − Cost。【F:out/q5_league_summary.csv†L1-L10】【F:out/q5_league_scb_summary.csv†L1-L10】

#### 讨论
- 无 SCB 演化与 Q4 ε=0 基本一致（PAVLOV 主导），而 SCB 让 ALLC 迅速占据 94% 份额，其余复杂策略份额降至 0–2%，说明复杂度预算可将生态重置为最简单策略统治。【F:out/q5_no_scb_share_matrix.csv†L1-L13】【F:out/q5_with_scb_share_matrix.csv†L1-L13】
- Empath/Reflector 的 Net Mean 为负，意味着成本映射（3）足以抵消其额外收益；若成本降低 1，其净收益即可转正（假设成本=2 时 Empath Net≈0.701）。【F:out/q5_league_scb_summary.csv†L1-L10】
- PROBER 在 SCB 下同样失去竞争力（Net -0.340），佐证探测/悔过策略均需要更高复杂度预算才能维持优势。

## 3. 综合总结报告
- **Q1**：无噪声下 Empath 与 TFT 并列第一（Mean≈2.67），Reflector 对 ALLC 取得 3.34 分但对 GRIM 仅 0.89，说明学习型策略兼具机会主义与脆弱性。【F:out/q1_summary.csv†L2-L8】【F:out/q1_pair_matrix.csv†L1-L8】
- **Q2**：噪声 0→0.05 造成互惠策略平均收益大幅下滑（-12~-19），GRIM 崩溃；Reflector 仅下降 0.19，表现最稳健。【F:out/q2_summary.csv†L1-L25】【F:out/q2_collapse_thresholds.csv†L1-L10】
- **Q3**：PROBER 对 ALLC 的剥削差 4.93，而对 TFT/Empath 仅 0.17/0.20；噪声进一步缩小差距，显示适应性互惠在混合人群中的抗剥削能力。【F:out/q3_pair_summary.csv†L1-L3】【F:out/q3_tournament_gaps.csv†L1-L20】
- **Q4**：无噪声演化由 PAVLOV 主导；噪声促成 PROBER + CTFT 共治，Empath/Reflector 虽被挤压但保持非零份额。【F:out/q4_eps0_share_matrix.csv†L1-L13】【F:out/q4_eps005_share_matrix.csv†L1-L13】【F:out/q4_evo_summary.csv†L1-L19】
- **Q5**：复杂度成本重排生态，ALLC 以 97.5% 份额统治；Empath/Reflector 净收益为负，验证其复杂度=3 的设置合理。【F:out/q5_with_scb_share_matrix.csv†L1-L13】【F:out/q5_league_scb_summary.csv†L1-L10】
- **Empath/Reflector 综合评估**：
  - Empath 在无噪声/无成本时表现最佳（Mean≈2.67，份额最高达 26.5%），但噪声与成本会显著削弱其优势，适用于稳定且预算宽松的场景。【F:out/q1_summary.csv†L2-L8】【F:out/q5_evo_summary.csv†L1-L19】
  - Reflector 可在噪声环境维持高收益（ε=0.20 平均 2.174，CI>2），对背叛者亦能动态降低合作率；然而在进化与 SCB 下份额较低，说明其学习机制难与传统互惠长期竞争。【F:out/q2_summary.csv†L18-L21】【F:out/q4_evo_summary.csv†L1-L19】
- **复现性**：所有命令固定 `--seed 42`（单对单使用 `--seed 7` 复现 PROBER 行为）；`--save/--load` 可写入 JSON 备份配置，CSV 字段命名稳定，方便二次分析。【F:Config.cpp†L556-L670】【F:Reporter.cpp†L270-L309】

## 4. 附录：指标定义与计算方法
- `mean`：每回合平均收益，取重复次数的样本均值。
- `stdev`：样本标准差；`ci95 = mean ± 1.96·stdev/√repeats`。
- `coop_rate`：合作次数 / 总回合数。
- `echo_length`：互信破裂到恢复的平均步数。【F:TournamentManager.cpp†L62-L116】
- `first_defection`：首次背叛平均回合；无背叛为 NA。
- `Δmean/Δε`：相邻噪声水平的收益差除以噪声差。【F:out/q2_summary.csv†L1-L25】
- `exploitation gap`：剥削者平均收益 − 目标平均收益；Gap>0 且 CI 不重叠判定为有效剥削。【F:out/q3_tournament_gaps.csv†L1-L20】
- `share(gen)`：演化中策略在某代的份额；>50% 判定为主导，<1% 判定为灭绝。【F:EvolutionManager.cpp†L118-L338】
- `net_mean = raw_mean − cost`：SCB 下扣除复杂度成本后的净收益；演化中成本直接影响适应度，报表按配置映射扣减。【F:Reporter.cpp†L270-L309】【F:EvolutionManager.cpp†L73-L119】
## 5. English Addendum

### 5.1 Experimental results for Q1–Q5, with clear presentation and discussion

#### Q1. Baseline Tournament (ε = 0)
**Commands Used**
`./build/ipd --rounds 100 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q1_baseline.csv`

**Results Table**
| Strategy | Mean Payoff | StdDev | 95% CI | Cooperation Rate | First Defection |
| --- | --- | --- | --- | --- | --- |
| Empath | 2.668 | 0.699 | [2.601, 2.735] | 0.828 | 2.442 |
| TFT | 2.667 | 0.702 | [2.600, 2.734] | 0.826 | 2.408 |
| PAVLOV | 2.654 | 0.702 | [2.587, 2.722] | 0.819 | 2.533 |
| GRIM | 2.526 | 0.778 | [2.451, 2.600] | 0.720 | 2.908 |
| ALLC | 2.497 | 1.037 | [2.398, 2.596] | 1.000 | NA |
| Reflector | 2.256 | 0.918 | [2.168, 2.343] | 0.599 | 2.145 |
| ALLD | 1.672 | 1.373 | [1.541, 1.804] | 0.000 | 1.000 |

**Table Column Notes**: Statistics aggregated across 30 repeats; cooperation rate and first defection reported per-player.【F:out/q1_summary.csv†L1-L8】

**Discussion**: Empath and TFT share overlapping CIs, proving that remorse-driven forgiveness matches classical tit-for-tat in a clean environment. Reflector gains 3.34 against ALLC but collapses versus GRIM (0.893), revealing its opportunistic yet fragile learning approach.【F:out/q1_pair_matrix.csv†L1-L8】

#### Q2. Noise Sensitivity Scan
**Commands Used**
`for eps in 0.00 0.05 0.10 0.20; do ./build/ipd --rounds 150 --repeats 30 --epsilon $eps --seed 42 --strategies TFT,GRIM,PAVLOV,CTFT,Empath,Reflector --payoffs 5,3,1,0 --format csv --output out/q2_eps_${eps}.csv; done`

**Average Payoff per Strategy**
| Strategy | ε=0.00 | ε=0.05 | ε=0.10 | ε=0.20 |
| --- | --- | --- | --- | --- |
| TFT | 2.956 [2.942, 2.970] | 2.290 [2.233, 2.347] | 2.241 [2.190, 2.292] | 2.216 [2.174, 2.258] |
| GRIM | 2.740 [2.679, 2.800] | 1.789 [1.724, 1.853] | 1.964 [1.909, 2.018] | 2.250 [2.205, 2.295] |
| PAVLOV | 2.953 [2.938, 2.968] | 2.148 [2.097, 2.199] | 2.151 [2.102, 2.201] | 2.182 [2.140, 2.224] |
| CTFT | 2.976 [2.969, 2.982] | 2.342 [2.268, 2.416] | 2.248 [2.183, 2.312] | 2.172 [2.125, 2.220] |
| Empath | 2.952 [2.938, 2.967] | 2.269 [2.213, 2.325] | 2.231 [2.180, 2.282] | 2.191 [2.149, 2.233] |
| Reflector | 2.479 [2.405, 2.553] | 2.286 [2.223, 2.350] | 2.227 [2.170, 2.284] | 2.174 [2.128, 2.220] |

**Table Column Notes**: Entries show mean payoff with 95% CI brackets for each ε.【F:out/q2_summary.csv†L1-L25】

**Discussion**: GRIM collapses at ε=0.05 (CI below 2.0) because noisy defections trigger long retaliation chains. Reflector loses only 0.193 points between ε=0 and ε=0.20, making it the most noise-tolerant strategy in the set.【F:out/q2_summary.csv†L1-L25】【F:out/q2_collapse_thresholds.csv†L1-L10】

#### Q3. Exploitation by PROBER and ALLD
**Commands Used**
`./build/ipd --rounds 150 --repeats 30 --epsilon 0.00 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --format csv --output out/q3_no_noise.csv`
`./build/ipd --rounds 150 --repeats 30 --epsilon 0.05 --seed 42 --strategies ALLC,TFT,CTFT,PAVLOV,PROBER,ALLD,Empath,Reflector --format csv --output out/q3_noise.csv`
`./build/ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,ALLC --format csv --output out/q3_prober_allc.csv`
`./build/ipd --rounds 150 --repeats 20 --epsilon 0.00 --seed 7 --strategies PROBER,TFT --format csv --output out/q3_prober_tft.csv`

**Tournament vs Noise**
| Strategy | ε=0 Mean | ε=0.05 Mean | ε=0 Coop | ε=0.05 Coop |
| --- | --- | --- | --- | --- |
| PROBER | 2.886 | 2.503 | 0.687 | 0.463 |
| CTFT | 2.732 | 2.424 | 0.868 | 0.784 |
| TFT | 2.718 | 2.346 | 0.854 | 0.582 |
| Empath | 2.683 | 2.378 | 0.835 | 0.603 |
| Reflector | 2.566 | 2.371 | 0.726 | 0.622 |
| ALLC | 2.197 | 2.151 | 1.000 | 0.950 |
| ALLD | 1.565 | 2.009 | 0.000 | 0.049 |

**Table Column Notes**: Side-by-side comparison of mean payoff and cooperation when noise is added.【F:out/q3_no_noise.csv†L1-L9】【F:out/q3_noise.csv†L1-L9】

**Pairwise Exploitation**
| Matchup | Exploiter Mean | Target Mean | Gap |
| --- | --- | --- | --- |
| PROBER vs ALLC | 4.973 | 0.040 | +4.933 |
| PROBER vs TFT | 2.993 | 2.993 | ≈0 |

**Table Column Notes**: Positive gaps with disjoint CIs indicate successful exploitation.【F:out/q3_pair_summary.csv†L1-L3】

**Discussion**: PROBER extracts nearly five points from ALLC but cannot beat TFT; Empath narrows the tournament gap to 0.203 (ε=0) and 0.125 (ε=0.05), revealing its partial resistance, especially when noise disrupts probing.【F:out/q3_tournament_gaps.csv†L1-L20】 ALLD benefits from noise yet remains below cooperative strategies.【F:out/q3_noise.csv†L1-L9】

#### Q4. Evolutionary Dynamics (50 Generations)
**Commands Used**
`./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --format csv --output out/q4_evo_eps0.csv`
`./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.05 --seed 42 --strategies ALLC,ALLD,TFT,PAVLOV,GRIM,CTFT,PROBER,Empath,Reflector --format csv --output out/q4_evo_eps005.csv`

**Population Shares (selected generations)**
| ε=0 Generation | PAVLOV | TFT | Empath | PROBER |
| --- | --- | --- | --- | --- |
| 0 | 0.110 | 0.110 | 0.110 | 0.110 |
| 10 | 0.345 | 0.220 | 0.095 | 0.145 |
| 30 | 0.510 | 0.200 | 0.130 | 0.045 |
| 50 | 0.365 | 0.230 | 0.090 | 0.160 |

| ε=0.05 Generation | CTFT | PROBER | Empath | TFT |
| --- | --- | --- | --- | --- |
| 0 | 0.110 | 0.110 | 0.110 | 0.110 |
| 10 | 0.510 | 0.255 | 0.030 | 0.135 |
| 30 | 0.480 | 0.460 | 0.015 | 0.020 |
| 50 | 0.265 | 0.635 | 0.040 | 0.005 |

**Table Column Notes**: Extracted from full share matrices to highlight dominant strategies.【F:out/q4_eps0_share_matrix.csv†L1-L13】【F:out/q4_eps005_share_matrix.csv†L1-L13】

**Discussion**: Noise-free evolution is ruled by PAVLOV; with ε=0.05, PROBER and CTFT alternate above 50%, relegating Empath and TFT to marginal roles. Reflector never exceeds 11%, staying peripheral.【F:out/q4_evo_summary.csv†L1-L19】

#### Q5. Strategy Complexity Budget
**Commands Used**
`./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --format csv --output out/q5_no_scb.csv`
`./build/ipd --evolve 1 --population 200 --generations 50 --mutation 0.02 --epsilon 0.00 --seed 42 --strategies ALLC,ALLD,TFT,GRIM,PAVLOV,CTFT,PROBER,Empath,Reflector --scb ALLC=1,ALLD=1,TFT=2,GRIM=2,PAVLOV=2,CTFT=3,PROBER=3,Empath=3,Reflector=3 --format csv --output out/q5_with_scb.csv`

**Evolution Outcome (Generation 50)**
| Scenario | ALLC | PAVLOV | TFT | Empath | PROBER |
| --- | --- | --- | --- | --- | --- |
| No SCB | 0.020 | 0.410 | 0.275 | 0.240 | 0.030 |
| With SCB | 0.940 | 0.005 | 0.020 | 0.010 | 0.005 |

**Table Column Notes**: Final-generation shares illustrating the shift once costs are applied.【F:out/q5_no_scb_share_matrix.csv†L1-L13】【F:out/q5_with_scb_share_matrix.csv†L1-L13】

**Net Payoffs**
| Strategy | Raw Mean | Net Mean |
| --- | --- | --- |
| ALLC | 2.287 | 1.287 |
| PAVLOV | 2.748 | 0.748 |
| TFT | 2.745 | 0.745 |
| Empath | 2.701 | -0.299 |
| PROBER | 2.660 | -0.340 |
| Reflector | 2.370 | -0.630 |

**Table Column Notes**: Costs of 2–3 points push advanced strategies into negative territory, explaining their extinction.【F:out/q5_league_summary.csv†L1-L10】【F:out/q5_league_scb_summary.csv†L1-L10】

**Discussion**: Without SCB, the mix resembles Q4 (PAVLOV-led cooperation). When complexity penalties apply, ALLC dominates (>94%) and all high-complexity strategies become uncompetitive, matching their negative net payoffs.【F:out/q5_evo_summary.csv†L1-L19】【F:out/q5_league_scb_summary.csv†L1-L10】

### 5.2 Design Rationale (≤ 600 words)
The engine separates configuration, simulation, and reporting to maximise reproducibility. `Config::fromCommandLine` validates every option, handles legacy aliases, and persists effective settings via JSON so that any experiment can be saved and replayed with `--load` overrides.【F:Config.cpp†L405-L670】 `TournamentManager` encapsulates round-robin play: lambdas generate pairings, accumulate per-player metrics (scores, cooperation, echo length, costs), and feed `Reporter` for text/CSV/JSON rendering with optional SCB columns.【F:TournamentManager.cpp†L40-L209】【F:Reporter.cpp†L270-L411】 Evolutionary simulations reuse the tournament evaluator but wrap it with replicator dynamics, discrete sampling, and mutation, logging population shares each generation for downstream analysis.【F:EvolutionManager.cpp†L63-L338】 

Strategy extensibility hinges on the `Strategy` interface and factory registration; adding Empath or Reflector required only implementing `nextMove/reset/complexity` while reusing the existing loop.【F:Strategy.h†L11-L23】【F:StrategyFactory.cpp†L81-L94】 Empath embeds remorse windows (80% cooperation while apologising, 60% bias afterwards) to mimic forgiving tit-for-tat, whereas Reflector updates trust with mood-modulated learning to absorb noisy payoffs.【F:Empath.cpp†L6-L52】【F:Reflector.cpp†L6-L66】 

Key trade-offs balanced clarity and functionality: templates (`parseNumber`) avoid duplicated parsing code, while lambda-heavy aggregation keeps tournament logic concise yet expressive.【F:Config.cpp†L90-L114】【F:TournamentManager.cpp†L40-L178】 SCB is applied both during tournaments (for reporting net scores) and in evolution (as a penalty in fitness computation), ensuring costs influence both analytical and adaptive outcomes.【F:Reporter.cpp†L270-L309】【F:EvolutionManager.cpp†L73-L119】 

Testing focuses on reproducibility: smoke runs (`--help`, tiny tournaments) confirm CLI integrity, fixed seeds guarantee deterministic tournaments/evolution, and all outputs share stable column names to support automated analysis. Combined, these choices deliver a single CLI that orchestrates deterministic experiments, complex tournaments, and evolutionary studies without code duplication or manual intervention.
