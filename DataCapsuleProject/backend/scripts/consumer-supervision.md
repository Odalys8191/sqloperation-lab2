# Consumer Service Supervision

## Systemd (recommended on VM/host)

```bash
sudo cp backend/scripts/consumer.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now consumer.service
sudo systemctl status consumer.service
```

## Docker restart policy (if consumer runs in container)

```bash
docker run --restart unless-stopped --name datacapsule-consumer <image>
```

Or for an existing container:

```bash
docker update --restart unless-stopped datacapsule-consumer
```
