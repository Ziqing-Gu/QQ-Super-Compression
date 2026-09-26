from pathlib import Path
root=Path(__file__).parent
def edit(name,pairs):
    p=root/name;s=p.read_text(encoding='utf-8-sig')
    for a,b in pairs:
        assert a in s,(name,a)
        s=s.replace(a,b)
    p.write_text(s,encoding='utf-8')
edit('tests/floor_checks.h',[
 ('const double floor=1e-6;','const double floor=std::pow(10.,-90./20);'),
 ('-119.99f','-89.99f'),('-120);const auto after','-90);const auto after'),('Classic -120dB','Classic -90dB')])
edit('tests/db_dynamics_test.cpp',[
 ('-90.0f,-110.0f','-80.0f,-90.0f'),('-10/-30/-90/-110','-10/-30/-80/-90'),
 ('setupDb(p,0,0,-110,1,0.125f)','setupDb(p,0,0,-90,1,0.125f)'),
 ('constexpr float amp=1e-5f','constexpr float amp=1e-4f'),('87.5','70.0'),
 ('dual?-119.9f:-110.0f,dual?-110.0f','dual?-90.0f:-80.0f,dual?-80.0f'),
 ('-110*.999','-80*.999'),('-109.89dB','-79.92dB')])
edit('tests/db_visual_checks.h',[
 ('false,false,0,-110','false,false,0,-90'),('checkPoint(-100,-12.5f)','checkPoint(-80,-10.0f)'),
 ('checkPoint(0,-109.89f)','checkPoint(0,-89.91f)'),('+87.5dB','+70dB')])
edit('tests/floor_visual_checks.h',[
 ('algorithm==0?-120+100/11.9','algorithm==0?-90+70/11.9'),
 ('algorithm==0?"-120.00 dB"','algorithm==0?"-90.00 dB"'),
 ('algorithm==0?"-120"','algorithm==0?"-90"'),
 ('false,false,2,-110','false,false,2,-80'),('setValue(-119,','setValue(-89,'),
 ('false,false,2)+109','false,false,2)+79'),('contains("-120")','contains("-90")'),
 ('Classic -120dB','Classic -90dB'),('99.9','69.93')])
edit('tests/ab_match_checks.h',[('99.9','69.93')])
edit('tests/algorithm_vst_check.cpp',[
 ('-119.99f','-89.99f'),('std::pow(1e-5,1-1/11.9)','std::pow(std::pow(10.,-70./20),1-1/11.9)'),
 ('algo==0?"-120"','algo==0?"-90"'),('-120dB endpoint','-90dB endpoint'),('new -120dB audio','new -90dB audio')])
edit('tests/visual_check.cpp',[
 ('!=qqsc::params::thresholdOffDb','!=(processor.isClassicAlgorithm()?qqsc::classicThresholdMinimumDb:qqsc::params::thresholdOffDb)'),
 ('Alt reset does not restore Dual -inf/0','Alt reset does not restore algorithm minimum/0'),
 ('actual Alt resets restore Dual -inf/0','actual Alt resets restore algorithm minimum/0')])
for name in ['package-candidate.ps1','install-candidate.ps1']:
    edit(name,[('1.2.5 Candidate','1.2.5 Candidate-Classic90')])
edit('install-candidate.ps1',[
 ('E56C48964156FCC69308F4E9B11930A73FF1020DCCF05550AA856E148A143534','6F4448378EB5FC1C5AB9F440203D6C06EA8B02BEE9A6E6575657D19593EEA280'),
 ('QQSC-Before-1.2.5-Candidate-','QQSC-Before-1.2.5-Classic90-')])
edit('TRY_1.2.5.md',[
 ('1.2.5 — 验证版','1.2.5 — Classic −90 dB 修订验证版'),
 ('有限的 **−120 dB**','有限的 **−90 dB**'),
 ('现在按 −120 dB 处理','现在按 −90 dB 处理')])
p=root/'TRY_1.2.5.md'
s=p.read_text(encoding='utf-8')
s+='\n本次修订（2026-09-27）：Classic 的 Single Threshold、Dual 上下阈值以及有限 Range 最低均为 −90 dB，推子下限、数值输入、Alt 复位、宿主读数和 DSP 一致。低于 −90 dB 的历史存储值在 Classic 中按 −90 dB 处理；保留宿主原参数范围以兼容 Super 的 −inf，未编辑的旧存储值可在切回 Super 时继续使用。\n'
p.write_text(s,encoding='utf-8')
edit('README.md',[('Classic −120 dB 最低阈值','Classic −90 dB 最低阈值')])
p=root/'CHANGELOG.md';s=p.read_text(encoding='utf-8');p.write_text('# 1.2.5 Candidate 修订 — 2026-09-27\n\n- 按用户要求将 Classic 最低阈值改为 −90 dB，Single / Dual、推子、读数、复位和 DSP 统一；Super 保持 −inf。\n- 保留宿主参数的历史存储范围；Classic 中低于下限的旧值按 −90 dB 处理。\n\n'+s,encoding='utf-8')
edit('CANDIDATE_CHECKPOINT_1.2.5.md',[
 ('1.2.5 Candidate`.','1.2.5 Candidate-Classic90`.'),
 ('Classic finite -120 dB floor','Classic finite -90 dB floor (2026-09-27 revision)'),
 ('installed 1.2.4 SHA','installed original 1.2.5 SHA')])
print('Updated Classic -90 dB tests and revision delivery scripts')
