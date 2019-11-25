  #include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include <SOEM.h>

// IOデータバッファ
static char IOmap[4096];
// 期待されるWKC値
static int expectedWKC;

// 開く
// nif: ネットワークインターフェースID
// return: 0=失敗 / 1=成功
int soem_open(char* nif)
{
	int ret = ec_init(nif);
	return ret;
}

// 閉じる
void soem_close(void)
{
	ec_close();
}

#define ALL_SLAVES_OP_STATE	0	// 全てのスレーブがOP状態になりました (成功)
#define NO_SLAVES_FOUND		1	// スレーブがみつかりません
#define NOT_ALL_OP_STATE	2	// OP状態にならないスレーブがあります

// コンフィグする
// return 結果
int soem_config(void)
{
	int oloop, iloop, chk;
	
	// スレーブを見つけ、自動コンフィグする
	if ( ec_config_init(FALSE) > 0 )
	{
		printf("%d slaves found and configured.\n",ec_slavecount);
		
		ec_config_map(&IOmap);
		ec_configdc();
		
		printf("Slaves mapped, state to SAFE_OP.\n");
		
		// 全てのスレーブが SAFE_OP 状態に達するのを待つ
		ec_statecheck(0, EC_STATE_SAFE_OP,	EC_TIMEOUTSTATE * 4);
		oloop = ec_slave[0].Obytes;
		if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
		if (oloop > 8) oloop = 8;
		iloop = ec_slave[0].Ibytes;
		if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
		if (iloop > 8) iloop = 8;
		printf("segments : %d : %d %d %d %d\n",
			ec_group[0].nsegments,
			ec_group[0].IOsegment[0],
			ec_group[0].IOsegment[1],
			ec_group[0].IOsegment[2],
			ec_group[0].IOsegment[3]);
		printf("Request operational state for all slaves\n");
		expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
		printf("Calculated workcounter %d\n", expectedWKC);
		
		// 全てのスレーブにOP状態を要求
		ec_slave[0].state = EC_STATE_OPERATIONAL;
		/* send one valid process data to make outputs in slaves happy*/ // ←意味不明
		ec_send_processdata();
		ec_receive_processdata(EC_TIMEOUTRET);
		ec_writestate(0);
		chk = 40;
		// 全てのスレーブがOP状態に達するのを待つ
		do
		{
			ec_send_processdata();
			ec_receive_processdata(EC_TIMEOUTRET);
			ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
		}
		while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
		
		if (ec_slave[0].state == EC_STATE_OPERATIONAL )
		{
			return ALL_SLAVES_OP_STATE;
		}
		else
		{
			return NOT_ALL_OP_STATE;
		}
	}
	else
	{
		return NO_SLAVES_FOUND;
	}
}

// スレーブの数を取得
// return: スレーブの数
int soem_getSlaveCount(void)
{
	return ec_slavecount;
}

// スレーブの状態を更新
// return: 全スレーブの中で最も低い状態
int soem_updateState(void)
{
	int ret = ec_readstate();
	return ret;
}

// スレーブの状態を取得
// slave: スレーブのインクリメンタルアドレス
// return: 状態
int soem_getState(int slave)
{
	return ec_slave[slave].state;
}

// スレーブのALステータスコードを取得
// slave: スレーブのインクリメンタルアドレス
// return: ALステータスコード
int soem_getALStatusCode(int slave)
{
	return ec_slave[slave].ALstatuscode;
}

// スレーブのALステータスの説明を取得
// slave: スレーブのインクリメンタルアドレス
// desc: ALステータスの説明 (最大31文字)
void soem_getALStatusDesc(int slave, char* desc)
{
	snprintf(desc, 31, "%s", ec_ALstatuscode2string( ec_slave[slave].ALstatuscode ));
}

// スレーブの状態変更を要求
void soem_requestState(int slave, int state)
{
	ec_slave[slave].state = state;
	ec_writestate(slave);
}

// スレーブの名前を取得
// slave: スレーブのインクリメンタルアドレス
// name: スレーブの名前 (最大31文字)
void soem_getName(int slave, char* name)
{
	snprintf(name, 31, "%s", ec_slave[slave].name );
}

// スレーブのベンダ番号/製品番号/バージョン番号を取得
// slave: スレーブのインクリメンタルアドレス
// id: {ベンダ番号, 製品番号, バージョン番号}
void soem_getId(int slave, unsigned long* id)
{
	id[0] = ec_slave[slave].eep_man;
	id[1] = ec_slave[slave].eep_id;
	id[2] = ec_slave[slave].eep_rev;
}

// PDO転送する
// return:  0=失敗 / 1=成功
int soem_transferPDO(void)
{
	ec_send_processdata();
	int wkc = ec_receive_processdata(EC_TIMEOUTRET);

	if(wkc >= expectedWKC){
		return 1;
	}else{
		return 0;
	}
}

// 汎用スレーブPDO入力
// slave: スレーブのインクリメンタルアドレス
// offset: オフセットアドレス
// return: 入力値
uint8_t soem_getInputPDO(int slave, int offset)
{
	uint8_t ret = 0;
	
	if(slave <= ec_slavecount)
	{
		ret = ec_slave[slave].inputs[offset];
	}
	return ret;
}

// 汎用スレーブPDO出力
// slave: スレーブのインクリメンタルアドレス
// offset: オフセットアドレス
// value: 出力値
void soem_setOutPDO(int slave, int offset, uint8_t value)
{
	if(slave <= ec_slavecount)
	{
		ec_slave[slave].outputs[offset] = value;
	}
}

// ロボットアーム制御
void robot_arm_ctrl(void)
{
	char ifname[] = "EthernetShield2"; // It's dummy
	
	int result = soem_open(ifname);
	if(result == 0)
	{
		printf("can not open network adapter!\n");
		return ;
	}
	result = soem_config();
	if(result == NO_SLAVES_FOUND)
	{
		printf("slaves not found!\n");
		return ;
	}
	if(result == NOT_ALL_OP_STATE)
	{
		printf("at least one slave can not reach OP state!\n");
		return ;
	}
	
	// 時間確認用
	int pin2_output = 0;
	pinMode(2, OUTPUT);
	digitalWrite(2, pin2_output);
	
	// サーボの角度 {肩, 肘, 手首, 指} (0-180deg)
	const int L_DEG[4] = {  0,  55, 180, 100}; // ボリューム=   0のときの角度 {右, 下, 下, 閉}
	const int H_DEG[4] = {180, 175,   0,  35}; // ボリューム=1023のときの角度 {左, 上, 上, 開}
	const int I_DEG[4] = { 90,  55,  90, 100}; // 初期角度 {中, 下, 中, 閉}
	uint8_t servo[4]; // ボリュームの目標角度
	for(int i=0;i<4;i++){
		servo[i] = I_DEG[i];
	}
	int init_cnt = 0; // 初期動作カウンタ
	while (1)
	{
		// 時間確認用
		pin2_output = 1 - pin2_output;
		digitalWrite(2, pin2_output);
		
		for(int i=0;i<4;i++){
			soem_setOutPDO(2, i, servo[i]);
		}
		soem_setOutPDO(2, 4, 0xA5);
		
		soem_transferPDO();

    // 初期動作(ゆっくり目標角度へ)
    bool init_flag = false;
    if(init_cnt < 180){
      init_cnt++;
      init_flag = true;
      delay(10);
    }
    // 各々のサーボについて
		for(int i=0;i<4;i++){
			// ボリューム値を目標角度へ換算
			uint8_t h = soem_getInputPDO(1, 2*i + 0);
			uint8_t l = soem_getInputPDO(1, 2*i + 1);
			uint16_t vol = ((uint16_t)h << 8) | (uint16_t)l;
			if(vol > 1023) vol = 1023;
			int deg = map(vol, 0, 1023, L_DEG[i], H_DEG[i]);
			// 初期動作(ゆっくり目標角度へ)
			if(init_flag){
				if     (servo[i] < deg) servo[i]++;
				else if(servo[i] > deg) servo[i]--;
			}
			// 通常動作
			else{
				servo[i] = deg;
			}
			Serial.print(servo[i]);
			Serial.print(",");
		}
		Serial.print("\n");
	}// while(true)

	// 切断処理
	soem_requestState(0, EC_STATE_INIT);
	soem_close();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
}

void loop()
{
  robot_arm_ctrl();
  
#if 0
    if(Serial.available() > 0){
        char c = Serial.read();
        switch(c){
        case 'e':
            robot_arm_ctrl();
            break;
        }
    }
#endif
}
