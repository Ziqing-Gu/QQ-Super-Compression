# Historical Rev4 audition guide

Rev4 was subsequently promoted to Stable on 2026-09-10. See STABLE_1.2.0.md. The following records the earlier audition delivery, not a current release hold.

# QQ Super Compression 1.2.0 Rev4 — 本机试听候选

此版本不是 Stable。用户在实际工程中发现 Rev3 的有限 Range 边界爆裂声，
已暂停 Plan C / D；没有推送 GitHub、创建标签、构建 Mac 或发布 Release。

本次调整：
- 有限 Range 内侧逐渐减少向下衰减，达到或超过 Range 仍严格为 0 dB 动态增益。
  过渡区最多约 6 dB；两个边界接近时缩窄。Range OFF 保留原有向下增益公式。
- 有限上压门槛采用同类内侧连续过渡，门槛以下保持原音量。
  两者均按检测电平计算，不增加 Attack、Release、时间等待或插件延迟。
  过渡区里的处理量有所改变，需要用原工程重新听测。
- Dual 的 Up / Down Ratio 旁各有 ON/OFF，默认 ON；LR/MS 各声道独立。
  关闭一侧保留该 Ratio、阈值和 LINK 关系，Display 的 Boost/Cut/Output 随之更新。
  关闭某侧不扩大另一侧的作用区间，两个阈值仍保留原来的交接逻辑。
- 开关采用 10 ms 的处理/未处理交叉淡化，重复点击从当前状态反向淡化。
  8x/16x 过采样保持同样时长。开关状态随工程和 A/B 保存，新实例默认开启。

建议试听：用此前发生爆裂声的同一段素材和相同参数比较；分别试听 Range OFF
及有限 Range。Dual 中先关掉一侧，再反向比较；Ratio LINK 可继续保留开启。
没有宣称任意素材或突然大幅电平跳变都完全无杂音；本次自动检查覆盖边界连续性、
反复跨 Range、开关 Crossfade、状态和 UI。实际工程听感仍需用户确认。

版本号仍为 1.2.0，state schema 14。23 页说明书为 Rev3 内容，待此次行为与布局
确认后再更新。冻结的历史备份未访问或修改。

源码：D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown
验证日志位于本目录 Verification，界面检查图位于 Preview。