# HESIA CoreLib Documentation (v0.2.0)
## Manuel d'ingénierie et d'intégration

---

### 1. Introduction
HESIA CoreLib est une bibliothèque système open source conçue pour la sécurité critique, le contrôle d'inférence et la cryptographie post-quantique (PQC) dans les environnements embarqués. Elle fournit une ABI C stable et des bindings pour plusieurs langages (C++, Python, Ada, Rust).

### 2. Architecture de Sécurité
La bibliothèque repose sur trois piliers :
- **PQC (Post-Quantum)** : Utilisation de ML-KEM et ML-DSA (Dilithium) pour résister aux futures attaques quantiques.
- **Lightweight Crypto** : Algorithmes natifs optimisés (ASCON-128, Ed25519) pour les plateformes sans accélération matérielle.
- **System Guard** : Watchdog et Inference-Guard pour surveiller l'intégrité des exécutions IA.

### 3. Guide d'intégration ADA
Le binding Ada se trouve dans `bindings/ada/hesia_core.ads`.

#### Exemple de Génération de Clé Dilithium (Ada)
```ada
with Hesia_Core;
with System;
-- ...
Alg : constant Hesia_Core.Sig_Algorithm := Hesia_Core.Sig_ML_DSA_65;
-- Récupération des tailles et allocation
Result := Hesia_Core.Sig_Keypair(Alg, PK'Address, PK_Len, SK'Address, SK_Len, Err'Access);
```

### 4. API de Maintenance
Nouveauté de la v0.2.0, l'API de maintenance permet d'auditer l'état du système :
- **Firmware Version** : Vérification du niveau de version.
- **Tamper Detection** : Indicateur de violation physique ou logicielle.
- **Security Patch** : Niveau de mise à jour de sécurité.

### 5. Utilisation du HESIA CLI
L'outil `hesia-cli` permet une gestion rapide depuis le terminal :
- `hesia-cli info` : Audit du système.
- `hesia-cli keygen ed25519` : Génération de clés classiques.
- `hesia-cli keygen mldsa65` : Génération de clés PQC.

### 6. Compilation pour ARM/Jetson
Utilisez le fichier de toolchain fourni pour la compilation croisée :
`cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=scripts/arm64-toolchain.cmake`

---
*© 2026 Hesia-Unit. Tous droits réservés.*
