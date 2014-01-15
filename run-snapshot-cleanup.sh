#!/bin/bash

config=$1

snap_dir=$(sed '/^snapshot_keep_dir=/!d;s/.*=//' $config)
keep=$(sed '/^snapshot_keep_days=/!d;s/.*=//' $config)
period=$(sed '/^snapshot_keep_period=/!d;s/.*=//;s/[-:]/ /g' $config)
begin_hour=$(echo $period | awk '{print $1}')
begin_minute=$(echo $period | awk '{print $2}')
end_hour=$(echo $period | awk '{print $3}')
end_minute=$(echo $period | awk '{print $4}')

while true
do
  snapshots=$(find $snap_dir -type f)
  for file in $snapshots;
  do
    #echo $file
    file_hour=$(date +%H -r $file)
    file_minute=$(date +%M -r $file)
    if [ $file_hour -lt $begin_hour ] || [ $file_hour -gt $end_hour ]; then
      echo "needless, remove "$file
      rm $file
      continue
    fi
    file_ts=$(date +%s -r $file)
    cur_ts=$(date +%s)
    kept_second=$(expr $cur_ts - $file_ts)
    kept_day=$(expr $kept_second / 86400)
    if [ $kept_day -gt $keep ]; then
      echo "expired, remove "$file
      rm $file
    fi
  done
  sleep 30
done
