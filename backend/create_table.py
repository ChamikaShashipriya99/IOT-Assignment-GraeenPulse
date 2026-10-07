import boto3

import config


def main():
    client = boto3.client("dynamodb", region_name=config.AWS_REGION)
    name = config.DYNAMODB_TABLE

    if name in client.list_tables()["TableNames"]:
        print("Table already exists:", name)
    else:
        client.create_table(
            TableName=name,
            KeySchema=[
                {"AttributeName": "device_id", "KeyType": "HASH"},
                {"AttributeName": "timestamp", "KeyType": "RANGE"},
            ],
            AttributeDefinitions=[
                {"AttributeName": "device_id", "AttributeType": "S"},
                {"AttributeName": "timestamp", "AttributeType": "S"},
            ],
            BillingMode="PAY_PER_REQUEST",
        )
        print("Creating table", name, "...")
        client.get_waiter("table_exists").wait(TableName=name)
        print("Table created")

    ttl = client.describe_time_to_live(TableName=name)["TimeToLiveDescription"]
    if ttl.get("TimeToLiveStatus") in ("ENABLED", "ENABLING"):
        print("TTL already on")
    else:
        client.update_time_to_live(
            TableName=name,
            TimeToLiveSpecification={"Enabled": True, "AttributeName": "expires_at"},
        )
        print("TTL turned on for expires_at, old readings are removed after", config.DATA_RETENTION_DAYS, "days")


if __name__ == "__main__":
    main()
