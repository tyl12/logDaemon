#curl --request POST -H "Content-Type:application/octet-stream" \
#     -H "devId:xm111" \
#     -H "filename:logfile_2018_04_03_10_52_19.log.gz" \
#     -H "time:1522717932778" \
#     -H "md5:52915cd0d09a5a7bd69c5bd48a983ed0" \
#     --data-binary @"logfile_2018_04_03_10_52_19.log.gz" \
#     http://118.126.96.129/test/interflow/locker/log/report

# curl -o -I -s -w "%{http_code}" --request POST -H "Content-Type:application/octet-stream" \
#     -H "devId:xm111" \
#     -H "filename:logfile_2018_04_03_10_52_19.log.gz" \
#     -H "time:1522717932778" \
#     -H "md5:52915cd0d09a5a7bd69c5bd48a983ed0" \
#     --data-binary @"logfile_2018_04_03_10_52_19.log.gz" \
#     http://118.126.96.129/test/interflow/locker/log/report


cnt=0
while true; do
    echo `date`
    sleep 0.01
    cnt=$(($cnt+1))
    echo $cnt
    if [ $cnt == "1000" ]; then
        exit
    fi
done



#curl -o -I -s -w "%{http_code}" --fail --request POST -H "Content-Type:application/octet-stream" \
#    -H "devId:xm111" \
#    -H "filename:logfile_2018_04_03_10_52_19.log.gz" \
#    -H "time:1522717932778" \
#    -H "md5:52915cd0d09a5a7bd69c5bd48a983ed0" \
#    --data-binary @"logfile_2018_04_03_10_52_19.log.gz" \
#    http://118.126.96.129/test/interflow/locker/log/report

#Out=$(curl -qSfsw '\n%{http_code}' --request POST -H "Content-Type:application/octet-stream" \
#    -H "devId:xm111" \
#    -H "filename:logfile_2018_04_03_10_52_19.log.gz" \
#    -H "time:1522717932778" \
#    -H "md5:52915cd0d09a5a7bd69c5bd48a983ed0" \
#    --data-binary @"logfile_2018_04_03_10_52_19.log.gz" \
#    http://118.126.96.129/test/interflow/locker/log/report)
#
#echo "-----"
#echo $?
#echo "-----"
#echo $Out
#echo "-----"
#
