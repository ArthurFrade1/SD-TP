import socket
import threading
import json
import base64
import os
import math
import hashlib
import time
import sys
import random
# Configurações globais baseadas na Tabela 1
BLOCK_SIZE = 1024 # 1 KB por padrão
HOST = '127.0.0.1'
class Peer:
 def __init__(self, port, is_seeder, filepath, neighbors):
 self.port = port
 self.is_seeder = is_seeder
 self.filepath = filepath
 self.neighbors = neighbors

 self.metadata = {}
 self.owned_blocks = set()
 self.lock = threading.Lock()

 # Cria um diretório isolado para este peer salvar/ler arquivos
(evita conflitos locais)
 self.workspace = f"peer_{self.port}"
 os.makedirs(self.workspace, exist_ok=True)
 self.local_filepath = os.path.join(self.workspace,
os.path.basename(filepath) if filepath else "downloaded_file")
 if self.is_seeder:
 self._init_seeder()
 def _init_seeder(self):
 """Inicializa as informações do arquivo caso este nó seja o
dono original."""
 filesize = os.path.getsize(self.filepath)
 total_blocks = math.ceil(filesize / BLOCK_SIZE)

 # Calcula o hash SHA-256 original
 sha256_hash = hashlib.sha256()
 with open(self.filepath, "rb") as f:
 for byte_block in iter(lambda: f.read(4096), b""):
 sha256_hash.update(byte_block)

 self.metadata = {
 "filename": os.path.basename(self.filepath),
 "filesize": filesize,
 "block_size": BLOCK_SIZE,
 "total_blocks": total_blocks,
 "file_hash": sha256_hash.hexdigest()
 }
 self.owned_blocks = set(range(total_blocks))
 # Copia o arquivo para o workspace do seeder
 os.system(f'cp "{self.filepath}" "{self.local_filepath}"')
 print(f"[SEEDER] Arquivo pronto: {self.metadata['filename']} |
{total_blocks} blocos | Hash: {self.metadata['file_hash']}")
 def start(self):
 """Inicia a thread do Servidor e a thread do Cliente
(Leecher)."""
 server_thread = threading.Thread(target=self._run_server,
daemon=True)
 server_thread.start()
 if not self.is_seeder:
 client_thread = threading.Thread(target=self._run_leecher)
 client_thread.start()
 client_thread.join() # Aguarda o download terminar

 server_thread.join() # Mantém o servidor rodando
 # ================= SERVIDOR =================
 def _run_server(self):
 """Ouve conexões de outros peers solicitando dados."""
 server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
 server.bind((HOST, self.port))
 server.listen(5)
 print(f"[SERVER] Escutando na porta {self.port}...")
 while True:
 conn, addr = server.accept()
 threading.Thread(target=self._handle_client, args=(conn,
addr), daemon=True).start()
 def _handle_client(self, conn, addr):
 with conn:
 try:
 data = conn.recv(4096).decode('utf-8')
 if not data: return

 request = json.loads(data)
 response = {}
 if request['type'] == 'METADATA_REQ':
 with self.lock:
 if self.metadata:
 response = {'type': 'METADATA_RES',
'metadata': self.metadata}
 else:
 response = {'type': 'ERROR', 'msg':
'Metadata not available'}
 elif request['type'] == 'BITMAP_REQ':
 with self.lock:
 response = {'type': 'BITMAP_RES', 'blocks':
list(self.owned_blocks)}
 elif request['type'] == 'BLOCK_REQ':
 idx = request['index']
 with self.lock:
 has_block = idx in self.owned_blocks

 if has_block:
 with open(self.local_filepath, "rb") as f:
 f.seek(idx * BLOCK_SIZE)
 chunk = f.read(BLOCK_SIZE)
 response = {
 'type': 'BLOCK_RES',
 'index': idx,
 'data':
base64.b64encode(chunk).decode('utf-8')
 }
 else:
 response = {'type': 'ERROR', 'msg': 'Block not
found'}
 conn.sendall(json.dumps(response).encode('utf-8'))
 except Exception as e:
 pass
 # ================= CLIENTE (LEECHER) =================
 def _send_request(self, target_port, request_dict):
 """Função auxiliar para enviar requisição a um vizinho."""
 try:
 with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as
s:
 s.settimeout(2.0)
 s.connect((HOST, target_port))
 s.sendall(json.dumps(request_dict).encode('utf-8'))
 data = s.recv(65536).decode('utf-8')
 return json.loads(data)
 except Exception:
 return None
 def _run_leecher(self):
 """Conecta aos vizinhos para obter metadados e baixar blocos
faltantes."""
 print("[LEECHER] Iniciando busca por metadados...")

 # 1. Obter Metadados
 while not self.metadata:
 for neighbor in self.neighbors:
 res = self._send_request(neighbor, {'type':
'METADATA_REQ'})
 if res and res.get('type') == 'METADATA_RES':
 with self.lock:
 self.metadata = res['metadata']
 self.local_filepath =
os.path.join(self.workspace, self.metadata['filename'])
 # Cria arquivo vazio com o tamanho correto
 with open(self.local_filepath, "wb") as f:
 f.truncate(self.metadata['filesize'])
 print(f"[LEECHER] Metadados recebidos de
{neighbor}. Total de blocos: {self.metadata['total_blocks']}")
 break
 time.sleep(2)
 total_blocks = self.metadata['total_blocks']
 # 2. Baixar Blocos
 while len(self.owned_blocks) < total_blocks:
 with self.lock:
 missing_blocks = [i for i in range(total_blocks) if i
not in self.owned_blocks]

 for neighbor in self.neighbors:
 if not missing_blocks: break

 # Pergunta quais blocos o vizinho tem
 bitmap_res = self._send_request(neighbor, {'type':
'BITMAP_REQ'})
 if not bitmap_res or bitmap_res.get('type') !=
'BITMAP_RES': continue

 neighbor_blocks = set(bitmap_res['blocks'])

 # Escolhe um bloco aleatório que está faltando e que o
vizinho possui
 available_to_download = list(set(missing_blocks) &
neighbor_blocks)
 if not available_to_download: continue

 target_block = random.choice(available_to_download)

 # Solicita o bloco
 block_res = self._send_request(neighbor, {'type':
'BLOCK_REQ', 'index': target_block})
 if block_res and block_res.get('type') == 'BLOCK_RES':
 chunk = base64.b64decode(block_res['data'])

 # Salva no arquivo diretamente na posição correta
 with self.lock:
 with open(self.local_filepath, "r+b") as f:
 f.seek(target_block * BLOCK_SIZE)
 f.write(chunk)
 self.owned_blocks.add(target_block)

 print(f"[LEECHER] Bloco {target_block} recebido de
{neighbor}. Progresso: {len(self.owned_blocks)}/{total_blocks}")
 missing_blocks.remove(target_block)
 time.sleep(0.5)
 print("[LEECHER] Download concluído! Verificando
integridade...")
 self._verify_file()
 def _verify_file(self):
 """Verifica se o hash do arquivo baixado corresponde ao
metadado original."""
 sha256_hash = hashlib.sha256()
 with open(self.local_filepath, "rb") as f:
 for byte_block in iter(lambda: f.read(4096), b""):
 sha256_hash.update(byte_block)

 calculated_hash = sha256_hash.hexdigest()
 original_hash = self.metadata['file_hash']

 if calculated_hash == original_hash:
 print(f"[SUCESSO] Arquivo remontado perfeitamente! Hash:
{calculated_hash}")
 else:
 print(f"[ERRO] Falha na integridade. Esperado:
{original_hash} | Calculado: {calculated_hash}")
if __name__ == "__main__":
 if len(sys.argv) < 3:
 print("Uso: python peer.py <porta> <seeder/leecher>
[caminho_arquivo] <vizinho1> <vizinho2> ...")
 sys.exit(1)
 my_port = int(sys.argv[1])
 mode = sys.argv[2].lower()
 is_seeder = mode == 'seeder'

 if is_seeder:
 filepath = sys.argv[3]
 neighbors = [int(p) for p in sys.argv[4:]]
 else:
 filepath = None
 neighbors = [int(p) for p in sys.argv[3:]]
 peer = Peer(my_port, is_seeder, filepath, neighbors)
 try:
 peer.start()
 except KeyboardInterrupt:
 print("\nEncerrando peer...
