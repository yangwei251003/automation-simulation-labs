"""Functional protocol model. This is not an MCU, electrical, or Proteus simulation."""
from dataclasses import dataclass
from functools import reduce
from operator import xor
from pathlib import Path
import json

def frame(*data):
    seq = [len(data), *data]
    return seq + [reduce(xor, seq, 0)]

def valid(data):
    return len(data) >= 2 and data[0] == len(data) - 2 and reduce(xor, data, 0) == 0

@dataclass
class Slave:
    address: int
    online: bool = True
    led: bool = False

class Network:
    def __init__(self):
        self.slaves = {1: Slave(1), 2: Slave(2)}
        self.log = []

    def execute(self, address, cmd):
        slave = self.slaves[address]
        for attempt in range(1, 4):
            self.log.append(f'M>S ADDRESS {address:02X} ninth=1 try={attempt}')
            if slave.online:
                self.log.append(f'S{address}>M ADDRESS_ACK {address:02X} ninth=0')
                break
        else:
            self.log.append(f'S{address} OFFLINE: no address acknowledgement after 3 attempts')
            return False
        command = frame(cmd)
        self.log.append('M>S COMMAND ' + ' '.join(f'{b:02X}' for b in command))
        assert valid(command)
        slave.led = bool(cmd)
        self.log.append(f'S{address}>M ACK 0F')
        if cmd:
            status = frame(0x23, ord(str(address)), 0x23)
            assert valid(status)
            self.log.append(f'S{address}>M STATUS ' + ' '.join(f'{b:02X}' for b in status))
            self.log.append('M>S STATUS_ACK 0F')
        return True

    def key(self, k):
        operations = {1: [(1,1)], 2: [(1,0)], 3: [(2,1)], 4: [(2,0)],
                      5: [(1,1),(2,1)], 6: [(1,0),(2,0)]}[k]
        outcomes = [self.execute(a,c) for a,c in operations]
        return {'key': k, 'ok': outcomes, 'led1': self.slaves[1].led, 'led2': self.slaves[2].led}

def main():
    n = Network()
    rows = [n.key(k) for k in range(1,7)]
    expected = [([True],True,False),([True],False,False),([True],False,True),
                ([True],False,False),([True,True],True,True),([True,True],False,False)]
    for row, exp in zip(rows, expected):
        assert (row['ok'],row['led1'],row['led2']) == exp
    n.slaves[2].online = False
    rows += [n.key(3), n.key(5)]
    assert rows[-2]['ok'] == [False] and rows[-1]['ok'] == [True, False]
    assert valid([1,1,0]) and not valid([1,1,0xFE])
    assert frame(0x23,0x31,0x23)==[0x03,0x23,0x31,0x23,0x32]
    report = {'scope':'functional protocol model; not firmware or electrical circuit',
              'checks':'8 scenarios passed; XOR valid/invalid passed', 'results':rows, 'trace':n.log}
    out=Path(__file__).parent/'model_result.json'
    out.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('RS-485 FUNCTIONAL PROTOCOL MODEL (not circuit simulation)')
    print('KEY  RESULT       LED1 LED2')
    for row in rows:
        print(f"{row['key']:<4} {str(row['ok']):<12} {int(row['led1']):<4} {int(row['led2'])}")
    print('PASS: 8 model cases; XOR checks; details in model_result.json')

if __name__=='__main__': main()
