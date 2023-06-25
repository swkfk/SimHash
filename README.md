# 2023-BUAA-数据结构大作业-SimHash参考代码

竞速排行榜名次也不是很高，拓展性也大概率过不了，但我就是想分享一下。

## 各个分支释义

- `master   [**.***s]` 这个只有 README 和 LICENSE 的分支
- `dev      [**.***s]` 编写代码时使用的分支
- `dev-ex   [**.***s]` 拓展性版本，与同学本地对拍成功，但小数据运行时错误
- `store-01 [19.663s]` 初始版本，每篇文章使用一棵字典树
- `store-02 [18.558s]` 增加了批量 `fread` 的功能，封装了读写
- `store-03 [18.165s]` 通过批量 `malloc`，优化了时间
- `store-04 [12.498s]` 在读文章时，总树和分树同时访问；使用基于下标的 `qsort`
- `store-05 [ 8.781s]` 使用 `popcnt`；交换数组下标层次，便于缓存命中
- `store-06 [ 8.685s]` 在遍历字典树时，使用栈代替了递归
- `store-07 [ 7.139s]` Optimize the pointer index
- `store-08 [ 6.826s]` 文章只读一遍，在有效结点迁出数组
- `store-09 [ 4.495s]` 开启 `O3` 优化
- `store-10 [ 4.439s]` 优化停用词的读取
- `store-11 [ 2.144s]` 重构，使用数组字典树
- `store-12 [ 1.250s]` 计算指纹时对于词频为 0 的情况进行剪枝
- `store-13 [ 1.080s]` 记录文章中单词在字典树中的下标，第二趟直接遍历
- `store-14 [ 0.996s]` 不再记录文章单词的始末下标，使用哨兵元素进行优化 {最终提交版本，测试时长为 1.005 s}
- `store-15 [ 0.590s]` 使用多进程并行处理 {未使用}