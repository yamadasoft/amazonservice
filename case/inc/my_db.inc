<?php

include_once("errlog.inc");

class DBConnection {
    var $_conn = null;	// �R�l�N�V����
    /**
	 * @name DBConnection
     * @desc �R���X�g���N�^
     **/
    function DBConnection() {
		//
    }
    /**
     * @name logoutput
	 * @desc ���O�o�͏���
     */
	function logoutput($level, $message, $file=null, $line=null)
	{
		if (DEBUG) {
			$log = ErrLog::getInstance(null, 9);
		} else {
			$log = ErrLog::getInstance(null, 3);
		}
		//$log->ErrLog_Write($level, $message, $file, $line);
	}
    /**
     * @name getConnection
	 * @desc �f�[�^�x�[�X�A�N�Z�X�p�C���X�^���X�쐬
     */
    function &getConnection($DB_URI) {
        static $instance = null;
        if ($instance == null) {
            $instance = new DBConnection();
            // DB �ڑ�
            $ret = $instance->db_open($DB_URI);
			if (!$ret) {
				$instance = null;
			}
        }
        return $instance;
    }
	/**
	 * @name db_open
	 * @desc �f�[�^�x�[�X�I�[�v��
	 * @param $uri �f�[�^�x�[�X��`�i�z�X�g�ADB���A�A�J�E���g�Ȃǁj
	 * @return ���s����
	 **/
	function db_open($uri)
	{
		$this->_conn = mysql_connect($uri["host"], $uri["user"], $uri["pass"]);
		if (!$this->_conn) {
			$this->logoutput(ERRLOG_ERROR, "mysql_connect error.", __FILE__, __LINE__);
			return false;
		}
		mysql_select_db($uri["db"]);
		$sql = "SET NAMES utf8";
		$res = mysql_query($sql, $this->_conn);

		return true;
	}
	/**
	 * @name db_exec
	 * @desc SQL���s
	 * @param $sql ���s����SQL��
	 * @return ���s����
	 **/
	function db_exec($sql, $log=true)
	{
		if (DEBUG && $log) {
			$this->logoutput(LOG_DEBUG, $sql);// �f�o�b�O�p��SQL�����O��
		}
		$res = mysql_query($sql, $this->_conn);
		if (!$res) {
			$this->logoutput(LOG_ERROR, mysql_error($this->_conn), __FILE__, __LINE__);
		}
		return $res;
	}
	// �g�����U�N�V�����J�n
	function db_begin_transaction()
	{
		return $this->db_exec("BEGIN");
	}

	// �g�����U�N�V�����ُ�I��
	function db_rollback()
	{
		return $this->db_exec("ROLLBACK");
	}

	// �g�����U�N�V�����I��
	function db_commit()
	{
		return $this->db_exec("COMMIT");
	}
	/**
	 * @name escape_string
	 * @desc �f�[�^�x�[�X�̕����R�[�h�ϊ��ƃG�X�P�[�v
	 * @return �ϊ��㕶����
	 **/
	function escape_string($str, $raw=false)
	{
		if ($raw) {
			return mysql_escape_string($str);
		}
		if (DB_ENCODING == SCRIPT_ENCODING) {
			return mysql_escape_string($str);
		}
		return mysql_escape_string(mb_convert_encoding($str, DB_ENCODING, SCRIPT_ENCODING));
	}
	/**
	 * @name db_close
	 * @desc �f�[�^�x�[�X�N���[�Y
	 * @return �Ȃ�
	 **/
	function db_close()
	{
		mysql_close($this->_conn);
	}
	/**
	 * @name check_field_name
	 * @desc �L�[���[�h�Ɉ�v����t�B�[���h�����G�X�P�[�v����
	 * @param $name �t�B�[���h��
	 * @return �Ȃ�
	 **/
	function check_field_name($name)
	{
		if (($name == "case")||($name == "type")||($name == "check")||($name == "limit")) {	// �G�X�P�[�v���K�v
			return '`' . $name . '` ';
		}
		return $name;
	}
	/**
	 * @name make_where
	 * @desc �����̔z�񂩂�SQL��WHERE�����쐬����
	 * @param $where �����̔z��
	 * @return �쐬����WHERE��
	 **/
	function make_where($where)
	{
		$count = 0;
		$cond = "";
		if (isset($where)) {
			foreach ($where as $key => $val) {
				$field = $this->check_field_name($val["field"]);
				if ($count > 0) {
					if (isset($val['relation'])) {
						$cond .= " " . $val['relation'] . " ";
					} else {
						$cond .= " and ";
					}
				}
				if (isset($val['nest'])) {
					$cond .= "(" . $this->make_where($val['nest']) . ")";
				} else {
					if (($val['cond'] == 'in')||($val['cond'] == 'not in')) {
						if (isset($val['value'])) {
							if (is_array($val['value'])) {
								$cond .= $field . ' ' . $val['cond'] . " (" . $this->escape_string(implode($val['value'], ",")) . ")";
							} else {
								$cond .= $field . ' ' . $val['cond'] . " (" . $this->escape_string($val['value']) . ")";
							}
						}
						if (isset($val['select'])) {
							$sql = $this->make_select($val['select']["table"], $val['select']["where"], $val['select']["order"], $val['select']["page"], $val['select']["fields"]);
							$cond .= $field . ' ' . $val['cond'] . " (" . $sql . ")";
						}
					} else if ($val['cond'] == 'any') {	// pgsql�̂�
						$cond .= "'" . $this->escape_string($val['value']) . "'=any(" . $field . ")";
					} else if ($val['cond'] == 'all') {	// pgsql�̂�
						$cond .= "'" . $this->escape_string($val['value']) . "'=all(" . $field . ")";
/*
					} else if ($val['cond'] == 'is') {
						$cond .= $field . ' is ' . $val['value'];
*/
					} else {
						if ($val['cond'] == "") {
							$op = "=";
						} else {
							$op = $val['cond'];
						}
						if (isset($val['expression'])) {
							if (isset($val["field"])) {
								$cond .= $field . ' ' . $op . " " . $this->escape_string($val['expression']);
							} else {
								$cond .= $val['expression'];
							}
						} else {
							if ($val['value'] === "now()") {
								$cond .= $field . ' ' . $op . " sysdate()";
							} else {
								if (is_array($val['value'])) {
									if ($op == "=") {
										$rel = " or ";
									} else {
										$rel = " and ";
									}
									$cond .= "(";
									$n = 0;
									foreach ($val['value'] as $val2) {
										if ($n) {
											$cond .= $rel;
										}
										$cond .= $field . ' ' . $op . " '" . $this->escape_string($val2) . "'";
										$n++;
									}
									$cond .= ")";
								} else {
									$cond .= $field . ' ' . $op . " '" . $this->escape_string($val['value']) . "'";
								}
							}
						}
					}
				}
				$count++;
			}
		}
		return $cond;
	}
	/**
	 * @name insert_data
	 * @desc �f�[�^�̑}��
	 * @param $table �}������e�[�u����
	 * @param $data �}������f�[�^�z��i�z��̃L�[�����ږ��A�l���f�[�^�j
	 * @return ���s����
	 **/
	function insert_data($table, $data, $raw=false)
	{
		$value = '';
		$field = '';
		foreach ($data as $key => $val) {
			if ($value != "") {
				$value .= ",";
				$field .= ",";
			}
			$field .= $this->check_field_name($key);
			if (isset($val)) {
				if (is_array($val)) {
					if (count($val)) {
						$value .= "'{{";
						$cnt = 0;
						foreach ($val as $key2 => $val2) {
							if ($cnt > 0) {
								$value .= ",";
							}
							$value .= '"' . $this->escape_string($val2) . '"';
							$cnt++;
						}
						$value .= "}}'";
					} else {
						$value .= "NULL";
					}
				} else {
					if ($val === 'now()') {	// ���ݓ���
						$value .= "sysdate()";
					} else {
						$value .= "'" . $this->escape_string($val, $raw) . "'";
					}
				}
			} else {
				$value .= "NULL";
			}
		}
		if ($value != "") {
			$sql = "insert into " . $table . " (" . $field . ") values (" . $value . ")";
			if ($this->db_exec($sql, !$raw)) {
				$sql = "select last_insert_id() as id";
				$rt = $this->search_sql($sql);
				if ($rt["count"] > 0) {
					return $rt["data"][0]["id"];
				}
			}else{
				return false;
			}
		}
		return true;
	}
	/**
	 * @name update_data
	 * @desc �f�[�^�̍X�V
	 * @param $table �X�V����e�[�u����
	 * @param $data �X�V����f�[�^�z��i�z��̃L�[�����ږ��A�l���f�[�^�j
	 * @param $where �X�V����f�[�^����肷��WHERE����
	 * @return ���s����
	 **/
	function update_data($table, $data, $where, $raw=false)
	{
		$value = '';
		foreach ($data as $key => $val) {
			if ($value != "") {
				$value .= ",";
			}
			$field = $this->check_field_name($key);
			if (is_array($val)) {
				if (count($val)) {
					$value .= $field . "='{{";
					$cnt = 0;
					foreach ($val as $key2 => $val2) {
						if ($cnt > 0) {
							$value .= ",";
						}
						$value .= '"' . $this->escape_string($val2, $raw) . '"';
						$cnt++;
					}
					$value .= "}}'";
				} else {
					$value .= $field . "=NULL";
				}
			} else if ($val === 'now()') {	// ���ݓ����ɍX�V
				$value .= $field . "=sysdate()";
			//	$value .= $field . "=" . $val;
			} else if ($val === 'inc()') {	// �l���C���N�������g�i�ŗL�@�\�j
				$value .= $field . "=" . $val . '+1';
			} else if ($val === '') {
				$value .= $field . "=''";
			} else {
				$value .= $field . "='" . $this->escape_string($val, $raw) . "'";
			}
		}
		// WHERE�쐬
		$cond = $this->make_where($where);
		if ($value != "") {
			if ($cond != "") {
				$sql = "update " . $table . " set " . $value . " where " . $cond;
			} else {
				$sql = "update " . $table . " set " . $value;
			}
			return $this->db_exec($sql, !$raw);
		}
		return false;
	}
	/**
	 * @name delete_data
	 * @desc �f�[�^�̍폜
	 * @param $table �폜����e�[�u����
	 * @param $where �폜����f�[�^����肷��WHERE����
	 * @return ���s����
	 **/
	function delete_data($table, $where)
	{
		// WHERE�쐬
		$cond = $this->make_where($where);
		if ($cond != "") {
			$sql = "delete from " . $table . " where " . $cond;
			return $this->db_exec($sql);
		}
		return false;
	}
	/**
	 * @name count_data
	 * @desc �����Ɉ�v����f�[�^�̌����𓾂�
	 * @param $table �J�E���g����e�[�u����
	 * @param $where �J�E���g����f�[�^����肷��WHERE����
	 * @return �Y���f�[�^����
	 **/
	function count_data($tables, $where)
	{
		$data = array('count' => 0);
		if (is_array($tables)) {
			foreach ($tables as $v) {
				if ($v["join"]) {	// JOIN�̃e�[�u��
					$join .= " " . $v["join"] . " " . $v["table"];
					if ($v["col"]) {	// �����������̃t�B�[���h
						//$join .= " on " . $table . "." . $v["col"] . "=" . $v["table"] . "." . $v["col"];
						$join .= " using(" . $v["col"] . ")";
					} else if ($v["col1"] && $v["col2"]) {	// �ʖ��̃t�B�[���h
						$join .= " on " . $v["col1"] . "=" . $v["col2"];
					}
				} else if ($v["table"]) {	// ���C���̃e�[�u��
					$table = $v["table"];
				}
			}
		} else {
			$join = "";
			$table = $tables;
		}
//		$sql = "select count(*) from " . $table;
		$sql = "select count(*) from " . $table . $join;
		$cond = $this->make_where($where);
		if ($cond != "") {
			$sql .= " where " . $cond;
		}
		$ret = $this->db_exec($sql);
		if ($ret) {
			if (mysql_num_rows($ret)) {
				$row = @mysql_fetch_row($ret);
				$data['count'] = $row[0];
			}
		} else {
			$data['count'] = 0;
			$data['error'] = 1;
			$data['message'] = mysql_error($this->_conn);
		}
		return $data;
	}

	/**
	 * @name make_select
	 * @desc SELECT���̍쐬
	 * @param $table ��������e�[�u����
	 * @param $where ��������f�[�^����肷��WHERE����
	 * @param $order �f�[�^�̏o�͏��i�ȗ��j
	 * @param $page �o�͂���f�[�^�̈ʒu�ƌ����i�ȗ�����ƑS���j
	 * @param $fields �擾����t�B�[���h���i�ȗ�����ƑS�t�B�[���h�j
	 * @return sql��
	 **/
	function make_select($tables, $where, $order=NULL, $page=NULL, $fields=NULL)
	{
		if (is_array($tables)) {
			foreach ($tables as $v) {
				if ($v["join"]) {	// JOIN�̃e�[�u��
					$join .= " " . $v["join"] . " " . $v["table"];
					if ($v["col"]) {	// �����������̃t�B�[���h
						//$join .= " on " . $table . "." . $v["col"] . "=" . $v["table"] . "." . $v["col"];
						$join .= " using(" . $v["col"] . ")";
					} else if ($v["col1"] && $v["col2"]) {	// �ʖ��̃t�B�[���h
						$join .= " on " . $v["col1"] . "=" . $v["col2"];
					}
				} else if ($v["table"]) {	// ���C���̃e�[�u��
					$table = $v["table"];
				}
			}
		} else {
			$join = "";
			$table = $tables;
		}
		if (!$table) {	// �e�[�u�����w��
			echo "No Tables";
			return "";
		}
		if ($fields) {
			if (is_array($fields)) {
				$sql = "select " . join(",", $fields) . " from " . $table . $join;
			} else {
				$sql = "select " . $fields . " from " . $table . $join;
			}
		} else {
			$sql = "select * from " . $table . $join;
		}
		$cond = $this->make_where($where);
		if ($cond != "") {
			$sql .= " where " . $cond;
		}
		if (!is_null($order)) {
			$sql .= " order by ";
			$count = 0;
			foreach ($order as $key => $val) {
				if ($count > 0) {
					$sql .= ",";
				}
				$sql .= $val['field'];
				if (isset($val['desc'])) {
					$sql .= " desc";
				}
				$count++;
			}
		}
		if (!is_null($page)) {
			if (!isset($page['offset'])) {
				$page['offset'] = '0';
			}
		//	$sql .= " offset " . $page['offset'] . " limit " .  $page['limit'];
			$sql .= " limit " .  $page['offset'] . "," . $page['limit'];
		}
		return $sql;
	}
	/**
	 * @name search_data
	 * @desc �f�[�^�̌���
	 * @param $table ��������e�[�u����
	 * @param $where ��������f�[�^����肷��WHERE����
	 * @param $order �f�[�^�̏o�͏��i�ȗ��j
	 * @param $page �o�͂���f�[�^�̈ʒu�ƌ����i�ȗ�����ƑS���j
	 * @param $fields �擾����t�B�[���h���i�ȗ�����ƑS�t�B�[���h�j
	 * @return ���s����
	 **/
	function search_data($table, $where, $order=NULL, $page=NULL, $fields=NULL, $raw=false)
	{
		$sql = $this->make_select($table, $where, $order, $page, $fields);
		return $this->search_sql($sql, $raw);
	}

	/**
	 * @name search_sql
	 * @desc �f�[�^�̌���(SQL���ڎ��s)
	 *	�����e�[�u�����Q�Ƃ�����A���₢���킹���g�p����ꍇ�ɁA����SQL�����s����B
	 * @return ���s����
	 **/
	function search_sql($sql, $raw=false)
	{
		$ret = $this->db_exec($sql);
		if ($ret) {
			$data = array();
			$count = @mysql_num_rows($ret);
			for ($row = 0; $row < $count; $row++) {
				$data[$row] = mysql_fetch_assoc($ret);
				if (!$raw) {
					foreach ($data[$row] as $key => $val) {
						if (DB_ENCODING == SCRIPT_ENCODING) {
							//
						} else {
							$val = mb_convert_encoding($val, SCRIPT_ENCODING, DB_ENCODING);
						}
						// �z��f�[�^��W�J(���f�[�^�Ɂu{{�`}}�v���܂܂��ƃf�[�^���j�󂳂��̂Œ���)
						if (preg_match('/^{{(.+)}}/', $val, $match)) {
							$ary = explode(",", $match[1]);
							if ($ary) {
								$data[$row][$key] = $ary;
								foreach ($data[$row][$key] as $key2 => $val2) {
									if (preg_match('/"(.+)"/', $val2, $match)) {
										$data[$row][$key][$key2] = $match[1];
									}
								}
							}
						} else {
							$data[$row][$key] = $val;
						}
					}
				}
			}
			$result['count'] = $count;
			$result['data'] = $data;
			return $result;
		} else {
			$data['count'] = 0;
			$data['error'] = 1;
			$data['message'] = mysql_error($this->_conn);
		}
		return $data;
	}
}
?>